// Copyright 2011 Emilie Gillet.
//
// Pattern layout and interpolation adapted from Mutable Instruments Grids and
// Dylan Bolink's Truchets pattern banks under the GNU GPL v3 or later. The
// generated node data (included below) carries its original license header.

#include "truchet_patterns.h"

#include <stddef.h>

#include "truchet_resources.inc"

// The first index is X and the second is Y. This is the curated topology from
// Truchets rather than numeric node order, which is what makes X/Y movement
// travel through musically related grooves.
static const uint8_t *const DRUM_MAP[3][5][5] = {
    {
        {node_10, node_8, node_0, node_9, node_11},
        {node_15, node_7, node_13, node_12, node_6},
        {node_18, node_14, node_4, node_5, node_3},
        {node_23, node_16, node_21, node_1, node_2},
        {node_24, node_19, node_17, node_20, node_22},
    },
    {
        {node_39, node_26, node_25, node_29, node_33},
        {node_30, node_31, node_27, node_28, node_34},
        {node_38, node_40, node_35, node_46, node_32},
        {node_45, node_36, node_43, node_42, node_48},
        {node_44, node_37, node_47, node_41, node_49},
    },
    {
        {node_73, node_52, node_72, node_70, node_71},
        {node_53, node_50, node_51, node_54, node_74},
        {node_55, node_59, node_56, node_57, node_58},
        {node_64, node_60, node_61, node_62, node_63},
        {node_68, node_69, node_67, node_65, node_66},
    },
};

static int clamp_pct(int value) {
    if (value < 0) return 0;
    if (value > 100) return 100;
    return value;
}

static uint8_t mix_u8(uint8_t a, uint8_t b, uint8_t balance) {
    uint32_t mixed = (uint32_t)a * (255u - balance) +
                     (uint32_t)b * balance;
    return (uint8_t)((mixed + 127u) / 255u);
}

const char *truchet_bank_name(int bank) {
    static const char *const NAMES[TRUCHET_BANKS] = {
        "OG", "ELEC", "BREAK",
    };
    if (bank < 0) bank = 0;
    if (bank >= TRUCHET_BANKS) bank = TRUCHET_BANKS - 1;
    return NAMES[bank];
}

uint8_t truchet_level(int bank, int x, int y, int instrument, int step) {
    if (bank < 0) bank = 0;
    if (bank >= TRUCHET_BANKS) bank = TRUCHET_BANKS - 1;
    if (instrument < 0) instrument = 0;
    if (instrument >= TRUCHET_INSTRUMENTS)
        instrument = TRUCHET_INSTRUMENTS - 1;
    if (step < 0) step = 0;
    step %= TRUCHET_STEPS;

    x = clamp_pct(x);
    y = clamp_pct(y);
    int family = bank;
    // Grids stores 32 internal subdivisions per bar, while TECLA receives one
    // clock per 16th note. Sample every other map point and repeat it across
    // two bars: the display and fill phrase stay 32 steps long without
    // halving the audible tempo.
    size_t map_step = (size_t)(step & 15) * 2u;
    size_t offset = (size_t)instrument * 32u + map_step;

    // Scale 0..100 over four cells. Keep the endpoint inside cell 3 so X/Y
    // at 100 reaches node 4 through a full-strength interpolation weight.
    int sx = x * 1024 / 100;
    int sy = y * 1024 / 100;
    if (sx > 1023) sx = 1023;
    if (sy > 1023) sy = 1023;
    int ix = sx >> 8;
    int iy = sy >> 8;
    uint8_t fx = (uint8_t)(sx & 0xff);
    uint8_t fy = (uint8_t)(sy & 0xff);

    uint8_t a = DRUM_MAP[family][ix][iy][offset];
    uint8_t b = DRUM_MAP[family][ix + 1][iy][offset];
    uint8_t c = DRUM_MAP[family][ix][iy + 1][offset];
    uint8_t d = DRUM_MAP[family][ix + 1][iy + 1][offset];
    return mix_u8(mix_u8(a, b, fx), mix_u8(c, d, fx), fy);
}

void truchet_base_masks(int bank, int x, int y,
                        uint32_t out[TRUCHET_INSTRUMENTS]) {
    for (int instrument = 0; instrument < TRUCHET_INSTRUMENTS; instrument++) {
        uint32_t mask = 0;
        for (int step = 0; step < TRUCHET_STEPS; step++) {
            if (truchet_level(bank, x, y, instrument, step) >
                TRUCHET_BASE_THRESHOLD)
                mask |= 1u << step;
        }
        out[instrument] = mask;
    }
}

float truchet_fill_probability(uint8_t level, int step, float density) {
    // A small deadband makes the physical knob stable around its center.
    const float center_high = 0.53f;
    if (density <= center_high || level == 0 ||
        level > TRUCHET_BASE_THRESHOLD)
        return 0.0f;
    if (density > 1.0f) density = 1.0f;
    if (step < 0) step = 0;
    if (step >= TRUCHET_STEPS) step = TRUCHET_STEPS - 1;

    float strength = (density - center_high) / (1.0f - center_high);
    float priority = level / (float)TRUCHET_BASE_THRESHOLD;
    float phase = step / (float)(TRUCHET_STEPS - 1);
    float phase2 = phase * phase;
    // The fourth-power curve keeps most of the phrase restrained, then makes
    // the final quarter increasingly likely to produce a turnaround.
    float end_boost = 1.0f + 1.5f * phase2 * phase2;
    float probability = strength * (0.04f + 0.12f * priority) * end_boost;
    return probability < 0.45f ? probability : 0.45f;
}

int truchet_should_hit(int bank, int x, int y, int instrument, int step,
                       float density, float random_value) {
    const float center_low = 0.47f;
    const float center_high = 0.53f;
    uint8_t level = truchet_level(bank, x, y, instrument, step);
    int core = level > TRUCHET_BASE_THRESHOLD;

    if (density < center_low) {
        if (!core || density <= 0.0f) return 0;
        // Thin by the bank's own velocity/priority value instead of rolling
        // every hit independently. Strong anchors survive longest and the
        // two repeated bars stay alike, so lower density remains a reduced
        // version of the groove rather than a freshly randomized pattern.
        float keep = density / center_low;
        int threshold = 255 - (int)(keep * (255 - TRUCHET_BASE_THRESHOLD));
        return level > threshold;
    }
    if (density <= center_high) return core;
    if (core) return 1;
    return random_value < truchet_fill_probability(level, step, density);
}
