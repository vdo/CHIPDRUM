// =============================================================================
// DRONER - chopped detuned drone
// All 3 voices around the root (V1 root, V2 sub, V3 detuned root),
// rhythmically chopped by gate patterns. CV1 = detune width in cents.
// Slow duty-cycle LFO adds movement to the timbre.
// Mix the three outputs to hear the detuning as beating.
// =============================================================================
#include <math.h>
#include <stdio.h>

#include "../gfx.h"
#include "../mode.h"
#include "../voice.h"
#include "chop.h"
#include "mode_util.h"

static int16_t p_chop = 0; // pattern select
static int16_t p_lfo = 3;  // duty LFO rate 0-10

static const param_t PARAMS[] = {
    {"CHOP", &p_chop, 0, NUM_CHOPS - 1, 1, chop_fmt},
    {"LFO", &p_lfo, 0, 10, 1, 0},
};

static float lfo_phase;
static float detune_cents;
static int cur_step;

static void reset(void) {
    lfo_phase = 0.0f;
    // start silent until the first tick
    for (int v = 0; v < 3; v++) voice_gate(v, false);
}

static void on_tick(uint64_t now) {
    cur_step = g.step % 16;
    chop_apply(p_chop, cur_step, now);
}

static void update(float dt, uint64_t now) {
    (void)now;
    detune_cents = g.cv1 * 30.0f;
    float f = root_freq();
    voice_freq(0, f);
    voice_freq(1, f * 0.5f);
    voice_freq(2, f * powf(2.0f, detune_cents / 1200.0f));

    if (p_lfo > 0) {
        lfo_phase += dt * (0.2f + p_lfo * 0.5f);
        if (lfo_phase > 6.2832f) lfo_phase -= 6.2832f;
        uint8_t duty = (uint8_t)(50.0f + 35.0f * sinf(lfo_phase));
        voice_duty(0, duty);
        voice_duty(1, duty);
        voice_duty(2, (uint8_t)(50.0f - 35.0f * sinf(lfo_phase)));
    } else {
        voice_duty(0, 50);
        voice_duty(1, 50);
        voice_duty(2, 50);
    }
}

static void draw(void) {
    // three bars breathing with the LFO
    for (int v = 0; v < 3; v++) {
        float ph = lfo_phase + v * 2.09f;
        int w = 30 + (int)(24.0f * sinf(ph));
        int y = 16 + v * 10;
        gfx_rect(4, y, 60, 8, 1);
        gfx_fill_rect(4, y, w > 2 ? w : 2, 8, 1);
    }
    // chop pattern
    for (int i = 0; i < 16; i++) {
        int x = 68 + i * 3;
        if ((CHOP_MASKS[p_chop] >> i) & 1u) gfx_fill_rect(x, 20, 2, 8, 1);
        if (i == cur_step) gfx_pixel(x + 1, 31, 1);
    }
    char buf[16];
    sprintf(buf, "det %dc", (int)detune_cents);
    gfx_text(68, 36, buf, 1);
}

const mode_t MODE_DRONER = {
    .name = "DRONER",
    .reset = reset,
    .on_tick = on_tick,
    .update = update,
    .draw = draw,
    .params = PARAMS,
    .n_params = 2,
};
