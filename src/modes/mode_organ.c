// =============================================================================
// ORGAN - harmonic-stack drone (drawbar / harmonium character)
// V1: fundamental - V2: pedal (sub octave, fifth below, or unison)
// V3: harmonic chosen by CV1 (2nd, 3rd, 4th, 5th, 6th, 8th)
// Each voice's duty is driven by its own LFO at an irrational rate ratio, so
// the timbres drift out of step and the stack shimmers instead of pulsing.
// Mix the three outputs for the full registration.
// =============================================================================
#include <math.h>
#include <stdio.h>

#include "../gfx.h"
#include "../mode.h"
#include "../voice.h"
#include "chop.h"
#include "mode_util.h"

static int16_t p_chop = 0;
static int16_t p_pedal = 0; // V2 role
static int16_t p_shim = 4;  // shimmer (LFO) rate

static const char *const PEDAL_NAMES[3] = {"SUB", "5TH DN", "UNISON"};
static const float PEDAL_RATIO[3] = {0.5f, 0.6667f, 1.0f};

// Drawbar registrations for V3, as harmonic multiples of the fundamental
static const int HARMONICS[6] = {2, 3, 4, 5, 6, 8};
static const char *const HARM_NAMES[6] = {"8'", "5 1/3'", "4'",
                                          "3 1/5'", "2 2/3'", "2'"};

static void fmt_pedal(char *out, int16_t v) { sprintf(out, "%s", PEDAL_NAMES[v]); }

static const param_t PARAMS[] = {
    {"CHOP", &p_chop, 0, NUM_CHOPS - 1, 1, chop_fmt},
    {"PEDAL", &p_pedal, 0, 2, 1, fmt_pedal},
    {"SHIMMER", &p_shim, 0, 10, 1, 0},
};

static float ph[3];
static int harm_idx;
static int cur_step;
static uint8_t duty_now[3];

static void reset(void) {
    ph[0] = 0.0f;
    ph[1] = 1.7f;
    ph[2] = 3.4f;
    for (int v = 0; v < 3; v++) voice_gate(v, false);
}

static void on_tick(uint64_t now) {
    cur_step = g.step % 16;
    chop_apply(p_chop, cur_step, now);
}

static void update(float dt, uint64_t now) {
    (void)now;
    float f = root_freq();

    // CV1 sweeps the drawbar registration of voice 3
    int idx = (int)(g.cv1 * 6.0f);
    if (idx > 5) idx = 5;
    harm_idx = idx;

    voice_freq(0, f);
    voice_freq(1, f * PEDAL_RATIO[p_pedal]);
    voice_freq(2, f * (float)HARMONICS[harm_idx]);

    if (p_shim > 0) {
        // irrational-ish rate ratios keep the three LFOs from ever relocking
        static const float RATE[3] = {1.0f, 1.31f, 1.73f};
        float base = 0.05f + p_shim * 0.12f;
        for (int v = 0; v < 3; v++) {
            ph[v] += dt * base * RATE[v];
            if (ph[v] > 6.2832f) ph[v] -= 6.2832f;
            // stay clear of 50%: a square loses its even harmonics, so the
            // shimmer is strongest when the pulse width never sits centred
            duty_now[v] = (uint8_t)(38.0f + 26.0f * sinf(ph[v]));
            voice_duty(v, duty_now[v]);
        }
    } else {
        for (int v = 0; v < 3; v++) {
            duty_now[v] = 50;
            voice_duty(v, 50);
        }
    }
}

static void draw(void) {
    // drawbars: three sliders whose travel shows each voice's pulse width
    const char *labels[3] = {"F", "P", "H"};
    for (int v = 0; v < 3; v++) {
        int x = 10 + v * 16;
        gfx_rect(x, 14, 10, 34, 1);
        int h = (duty_now[v] * 30) / 100;
        if (h < 2) h = 2;
        gfx_fill_rect(x + 1, 14 + 32 - h, 8, h, 1);
        gfx_text(x + 2, 50, labels[v], 1);
    }
    char buf[16];
    gfx_text(64, 14, "DRAWBAR", 1);
    sprintf(buf, "%s", HARM_NAMES[harm_idx]);
    gfx_text(64, 26, buf, 1);
    sprintf(buf, "x%d", HARMONICS[harm_idx]);
    gfx_text(64, 36, buf, 1);
    // chop pattern strip
    for (int i = 0; i < 16; i++) {
        int x = 64 + i * 4;
        if ((CHOP_MASKS[p_chop] >> i) & 1u) gfx_fill_rect(x, 46, 3, 4, 1);
        if (i == cur_step) gfx_pixel(x + 1, 51, 1);
    }
}

const mode_t MODE_ORGAN = {
    .name = "ORGAN",
    .reset = reset,
    .on_tick = on_tick,
    .update = update,
    .draw = draw,
    .params = PARAMS,
    .n_params = 3,
};
