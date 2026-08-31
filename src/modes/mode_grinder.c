// =============================================================================
// GRINDER - beating / dissonant drone
// V1: root - V2: sub, beating slowly against itself - V3: root + INTERVAL,
// offset by CV1 in HERTZ rather than cents, so the beat rate stays constant
// as you transpose (the musically useful control for drones).
// PULSE narrows the duty for a reedy, nasal grind.
// Mix the outputs: the beating happens between voices.
// =============================================================================
#include <math.h>
#include <stdio.h>

#include "../gfx.h"
#include "../mode.h"
#include "../voice.h"
#include "chop.h"
#include "mode_util.h"

static int16_t p_chop = 0;
static int16_t p_interval = 0;
static int16_t p_pulse = 50; // duty %

static const char *const IVAL_NAMES[4] = {"UNISON", "MINOR2", "TRITONE", "FIFTH"};
static const int IVAL_SEMI[4] = {0, 1, 6, 7};

static void fmt_ival(char *out, int16_t v) { sprintf(out, "%s", IVAL_NAMES[v]); }
static void fmt_pct(char *out, int16_t v) { sprintf(out, "%d%%", v); }

static const param_t PARAMS[] = {
    {"CHOP", &p_chop, 0, NUM_CHOPS - 1, 1, chop_fmt},
    {"INTERVAL", &p_interval, 0, 3, 1, fmt_ival},
    {"PULSE", &p_pulse, 5, 50, 5, fmt_pct},
};

static float beat_hz;
static float beat_ph; // 0..1 position within one beat cycle, for the display
static int cur_step;

static void reset(void) {
    beat_ph = 0.0f;
    for (int v = 0; v < 3; v++) voice_gate(v, false);
}

static void on_tick(uint64_t now) {
    cur_step = g.step % 16;
    chop_apply(p_chop, cur_step, now);
}

static void update(float dt, uint64_t now) {
    (void)now;
    beat_hz = 0.05f + g.cv1 * g.cv1 * 8.0f; // squared: fine control when slow
    float f = root_freq();

    voice_freq(0, f);
    // sub beats at half the rate, keeping the low end moving too
    voice_freq(1, f * 0.5f + beat_hz * 0.5f);
    voice_freq(2, semi_freq(IVAL_SEMI[p_interval]) + beat_hz);

    for (int v = 0; v < 3; v++) voice_duty(v, (uint8_t)p_pulse);

    beat_ph += dt * beat_hz;
    if (beat_ph > 1.0f) beat_ph -= 1.0f;
}

static void draw(void) {
    // two traces drifting out of phase: this IS the beat
    int mid = 30;
    for (int x = 0; x < 128; x++) {
        float a = x * 0.19f;
        int y1 = mid + (int)(9.0f * sinf(a));
        int y2 = mid + (int)(9.0f * sinf(a * 1.06f + beat_ph * 6.2832f));
        gfx_pixel(x, y1, 1);
        gfx_pixel(x, y2, 1);
    }
    // beat indicator: swells once per beat cycle
    int r = 2 + (int)(4.0f * (0.5f - 0.5f * cosf(beat_ph * 6.2832f)));
    gfx_circle(118, 15, r, 1);

    char buf[20];
    if (beat_hz < 1.0f)
        sprintf(buf, "beat %.2fHz", (double)beat_hz);
    else
        sprintf(buf, "beat %.1fHz", (double)beat_hz);
    gfx_text(0, 14, buf, 1);
    gfx_text(0, 45, IVAL_NAMES[p_interval], 1);
    // chop strip
    for (int i = 0; i < 16; i++) {
        int x = 64 + i * 4;
        if ((CHOP_MASKS[p_chop] >> i) & 1u) gfx_fill_rect(x, 45, 3, 5, 1);
        if (i == cur_step) gfx_pixel(x + 1, 51, 1);
    }
}

const mode_t MODE_GRINDER = {
    .name = "GRINDER",
    .reset = reset,
    .on_tick = on_tick,
    .update = update,
    .draw = draw,
    .params = PARAMS,
    .n_params = 3,
};
