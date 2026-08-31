// =============================================================================
// TECLA BASS - generative bass/rhythm synth voice for Eurorack
// Raspberry Pi Pico (RP2040), Pico SDK
//
// Outputs: 3 PWM square-wave oscillators (GP22 bass, GP2 sub, GP0 accent)
//          master clock out on GP1
// Inputs:  external clock on GP27 (GPIO IRQ), CV1 on GP26, tempo slider GP28
// Core 0:  clock engine, modes, voices, buttons (tight timing)
// Core 1:  OLED rendering
// =============================================================================
#include <stdio.h>
#include <string.h>

#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "buttons.h"
#include "clockgen.h"
#include "config.h"
#include "drumsynth.h"
#include "hardware.h"
#include "mode.h"
#include "persist.h"
#include "rng.h"
#include "sampler.h"
#include "state.h"
#include "ui.h"
#include "voice.h"

state_t g = {
    .mode_idx = 0,
    .root = 0,   // C
    .octave = 1, // C2
    .gate_pct = 50,
};

uint32_t g_rng_state = 0xC0FFEE21u;

static void toast(const char *msg, const char *detail, uint32_t ms) {
    strncpy(g.toast, msg, sizeof(g.toast) - 1);
    g.toast[sizeof(g.toast) - 1] = 0;
    strncpy(g.toast_detail, detail, sizeof(g.toast_detail) - 1);
    g.toast_detail[sizeof(g.toast_detail) - 1] = 0;
    g.toast_until = time_us_64() + (uint64_t)ms * 1000;
}

static void switch_mode(int new_idx) {
    // neutralise per-mode voice settings before handing over. Sample mode is
    // released first so the incoming mode's reset() can claim it if it wants.
    for (int v = 0; v < NUM_VOICES; v++) {
        sampler_enable(v, false);
        voice_gate(v, false);
        voice_glide(v, 0.0f);
        voice_duty(v, 50);
    }
    MODES[new_idx]->reset();
    g.mode_idx = (int16_t)new_idx; // core1 picks the new mode up after reset
    g.param_idx = 0;
    led_show_param_slot(0);
}

int main(void) {
    stdio_init_all();

    hardware_init();
    voice_init_all();
    drumsynth_init();
    sampler_init();
    clock_init();
    persist_load();

    // seed the PRNG from the ADC noise floor
    for (int i = 0; i < 16; i++)
        g_rng_state = (g_rng_state << 3) ^ adc_read_cv1() ^ (g_rng_state >> 5);
    if (g_rng_state == 0) g_rng_state = 0xC0FFEE21u;

    MODES[g.mode_idx]->reset();
    multicore_launch_core1(ui_core1_entry);
    led_startup_animation();
    printf("TECLA BASS ready, mode %s\n", MODES[g.mode_idx]->name);

    uint64_t last_us = time_us_64();
    uint64_t t_buttons = 0, t_adc = 0;

    while (true) {
        uint64_t now = time_us_64();
        float dt = (now - last_us) * 1e-6f;
        last_us = now;

        // --- analog inputs, smoothed (every 1 ms) ---
        if (now - t_adc > 1000) {
            t_adc = now;
            float cv1 = adc_read_cv1() / 4095.0f;
            float sld = adc_read_slider() / 4095.0f;
            g.cv1 += 0.1f * (cv1 - g.cv1);
            g.slider += 0.1f * (sld - g.slider);
        }

        // --- clock + mode ticks ---
        int ticks = clock_task(now, g.slider);
        const mode_t *mode = MODES[g.mode_idx];
        for (int t = 0; t < ticks; t++) {
            g.step++;
            mode->on_tick(now);
        }
        if (mode->update) mode->update(dt, now);
        voice_update(now, dt);

        // clock LEDs
        led_set(0, clock_is_external());
        led_set(1, ticks > 0 || gpio_get(PIN_CLK_OUT));

        // --- buttons (every 5 ms) ---
        if (now - t_buttons > 5000) {
            t_buttons = now;
            int arg = 0;
            switch (buttons_task(now, &arg)) {
            case ACT_MODE_CHANGED:
                switch_mode(arg);
                persist_mode_changed(now);
                break;
            case ACT_RESET:
                mode->reset();
                break;
            case ACT_RESTART:
                if (mode->restart)
                    mode->restart();
                else
                    mode->reset();
                toast("BAR RESET", "", 700);
                break;
            case ACT_RANDOMIZE:
                if (mode->randomize) {
                    int16_t x, y;
                    char xy[12];
                    mode->randomize(&x, &y);
                    snprintf(xy, sizeof(xy), "%d/%d", x, y);
                    toast("RAND X/Y", xy, 1100);
                }
                break;
            case ACT_SAVE:
                toast(persist_save() ? "SAVED" : "ERROR", "", 900);
                break;
            default:
                break;
            }
        }
        persist_task(now);
    }
}
