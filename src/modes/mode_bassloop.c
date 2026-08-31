// =============================================================================
// BASSLOOP - Turing-machine bass riff
// 16-bit shift register; gate = outgoing bit, pitch from 2 register bits
// mapped to a root-heavy set {root, root, -5, +12}. CV1 mirrored lock/mutate.
// V2: sub with V1 - V3: sparse fifth
// =============================================================================
#include <math.h>
#include <stdio.h>

#include "../gfx.h"
#include "../mode.h"
#include "../rng.h"
#include "../voice.h"
#include "mode_util.h"

static int16_t p_len = 8;

static uint16_t reg;
static float mutate_p;
static int last_offset;

static const param_t PARAMS[] = {
    {"LENGTH", &p_len, 2, 16, 1, 0},
};

static const int OFFSETS[4] = {0, 0, -5, 12};

static void reset(void) { reg = (uint16_t)rng_u32(); }

static void on_tick(uint64_t now) {
    int n = p_len;
    float k = g.cv1;
    uint32_t bit = (reg >> (n - 1)) & 1u;
    if (k > 0.5f) bit ^= 1u; // inverted branch: double-length loop at full CW
    mutate_p = 1.0f - fabsf(2.0f * k - 1.0f); // 0 at ends, 1 at noon
    if (rng_f() < mutate_p) bit = rng_bit();
    reg = (uint16_t)((reg << 1) | bit);

    uint32_t gms = gate_ms();
    if (bit) {
        last_offset = OFFSETS[(reg >> 1) & 3u];
        voice_freq(0, semi_freq(last_offset));
        voice_gate_ms(0, gms, now);
        voice_freq(1, semi_freq(last_offset - 12));
        voice_gate_ms(1, gms, now);
        if ((reg >> 3) & 1u) {
            voice_freq(2, semi_freq(7));
            voice_gate_ms(2, gms / 2, now);
        }
    }
}

static void draw(void) {
    int n = p_len, cx = 30, cy = 32, r = 16;
    for (int i = 0; i < n; i++) {
        float a = -1.5708f + 6.2832f * i / n;
        int px = cx + (int)(r * cosf(a));
        int py = cy + (int)(r * sinf(a));
        if ((reg >> i) & 1u)
            gfx_fill_rect(px - 1, py - 1, 3, 3, 1);
        else
            gfx_pixel(px, py, 1);
    }
    char buf[16];
    sprintf(buf, "L%d", n);
    gfx_text(58, 18, buf, 1);
    sprintf(buf, "mut %d%%", (int)(mutate_p * 100));
    gfx_text(58, 30, buf, 1);
    const char *names[4] = {"I", "I", "V.", "I+"};
    (void)names;
    sprintf(buf, "%+d st", last_offset);
    gfx_text(58, 42, buf, 1);
}

const mode_t MODE_BASSLOOP = {
    .name = "BASSLOOP",
    .reset = reset,
    .on_tick = on_tick,
    .update = 0,
    .draw = draw,
    .params = PARAMS,
    .n_params = 1,
};
