#include "clockgen.h"

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/time.h"

#include "config.h"
#include "edge_detect.h"
#include "scales.h"

// --- External clock input state ---
static uint64_t ext_last_edge_us = 0;
static uint32_t ext_period_us = 500000;
static uint32_t ext_pending_edges = 0;
static uint32_t ext_consumed_edges = 0;

// --- Internal clock ---
static uint64_t int_next_tick_us = 0;
static uint32_t int_period_us = 125000; // 120 BPM 16ths

// --- Divide / multiply ---
// Slider zones: /8 /6 /4 /3 /2 x1 x2 x3 x4
static const int FACTORS[] = {-8, -6, -4, -3, -2, 1, 2, 3, 4};
#define NUM_FACTORS 9
static int factor = 1;
static uint32_t div_count = 0;
// multiplication: extra sub-ticks scheduled after each incoming edge
static int sub_left = 0;
static uint64_t sub_next_us = 0;
static uint32_t sub_spacing_us = 0;

static bool was_external = false;
static int manual_pending = 0;

// --- Clock out pulse ---
static uint64_t pulse_off_at = 0;

// --- Adaptive analog edge detection (logic lives in edge_detect.c) ---
static edge_t clk_edge;
static uint16_t clk_now = 0;
static uint64_t clk_decay_at = 0;

static void clk_poll(uint64_t now) {
    adc_select_input(ADC_CLK_IN);
    uint16_t v = adc_read();
    clk_now = v;

    bool decay_now = now >= clk_decay_at;
    if (decay_now) clk_decay_at = now + CLK_DECAY_INTERVAL_US;

    if (edge_feed(&clk_edge, v, decay_now)) {
        uint64_t dt = now - ext_last_edge_us;
        if (dt >= EXT_PERIOD_MIN_US) { // lockout against contact bounce
            if (dt <= EXT_PERIOD_MAX_US) ext_period_us = (uint32_t)dt;
            ext_last_edge_us = now;
            ext_pending_edges++;
        }
    }
}

float clock_in_volts(void) { return clk_now * 3.3f / 4095.0f; }
float clock_in_swing_volts(void) {
    return edge_swing(&clk_edge) * 3.3f / 4095.0f;
}
bool clock_in_level(void) { return clk_edge.level; }

void clock_init(void) {
    gpio_init(PIN_CLK_OUT);
    gpio_set_dir(PIN_CLK_OUT, GPIO_OUT);
    gpio_put(PIN_CLK_OUT, 0);

    // Analog, NOT digital: adc_gpio_init also disables the digital input
    // buffer and any pulls, so we do not load the jack's front end.
    adc_gpio_init(PIN_CLK_IN);
    edge_init(&clk_edge, CLK_MIN_SWING, CLK_ENVELOPE_DECAY);

    int_next_tick_us = time_us_64() + int_period_us;
}

static void pulse_out(uint64_t now) {
    gpio_put(PIN_CLK_OUT, 1);
    pulse_off_at = now + CLOCK_PULSE_US;
}

void clock_manual_tick(void) { manual_pending++; }

bool clock_is_external(void) {
    return (time_us_64() - ext_last_edge_us) < EXT_TIMEOUT_US;
}

int clock_factor(void) { return factor; }

uint32_t clock_period_us(void) {
    if (clock_is_external()) {
        uint32_t p = ext_period_us;
        if (factor > 1) return p / (uint32_t)factor;
        if (factor < 0) return p * (uint32_t)(-factor);
        return p;
    }
    return int_period_us;
}

float clock_bpm(void) {
    uint32_t p = clock_period_us();
    if (p == 0) return 0.0f;
    // one tick = 16th note -> BPM = 60e6 / (4 * period)
    return 15000000.0f / (float)p;
}

int clock_task(uint64_t now, float slider_norm) {
    clk_poll(now); // sample the external clock jack

    // end the clock-out pulse
    if (pulse_off_at && now >= pulse_off_at) {
        gpio_put(PIN_CLK_OUT, 0);
        pulse_off_at = 0;
    }

    int fire = 0;
    bool external = clock_is_external();

    if (external) {
        if (!was_external) { // fresh sync: start counting from this edge
            div_count = 0;
            sub_left = 0;
            ext_consumed_edges = ext_pending_edges - 1; // keep the edge that woke us
        }
        // slider selects the divide/multiply factor
        int idx = (int)(slider_norm * NUM_FACTORS);
        if (idx >= NUM_FACTORS) idx = NUM_FACTORS - 1;
        factor = FACTORS[idx];

        // Consume incoming edges. Subtract rather than zero the counter: an
        // edge arriving between the read and the update must not be lost.
        uint32_t edges = ext_pending_edges - ext_consumed_edges;
        if (edges) {
            ext_consumed_edges += edges;
            for (uint32_t e = 0; e < edges && fire < 4; e++) {
                if (factor < 0) { // divide: fire every -factor edges
                    if (div_count == 0) fire++;
                    div_count = (div_count + 1) % (uint32_t)(-factor);
                } else { // x1 or multiply: fire now, schedule the rest
                    fire++;
                    if (factor > 1) {
                        sub_left = factor - 1;
                        sub_spacing_us = ext_period_us / (uint32_t)factor;
                        sub_next_us = now + sub_spacing_us;
                    }
                }
            }
        }
        // multiplied sub-ticks between edges
        while (sub_left > 0 && now >= sub_next_us && fire < 4) {
            fire++;
            sub_left--;
            sub_next_us += sub_spacing_us;
        }
        // keep the internal clock parked to avoid a burst on release
        int_next_tick_us = now + int_period_us;
    } else {
        factor = 1;
        float bpm = slider_to_bpm(slider_norm);
        int_period_us = (uint32_t)(15000000.0f / bpm);
        while (now >= int_next_tick_us && fire < 4) {
            fire++;
            int_next_tick_us += int_period_us;
        }
        // resync if we fell too far behind (e.g. after a long stall)
        if (now > int_next_tick_us + (uint64_t)int_period_us * 4)
            int_next_tick_us = now + int_period_us;
    }
    was_external = external;

    if (manual_pending) {
        fire += manual_pending;
        manual_pending = 0;
    }

    if (fire > 0) pulse_out(now);
    return fire;
}
