#pragma once

#include <stdint.h>

// Swing is expressed in the conventional long-part percentage of an eighth
// note. 50% is straight; 75% delays the offbeat 16th by half a clock period.
static inline uint32_t drum_swing_delay_us(int swing_pct,
                                           uint32_t tick_period_us,
                                           int step) {
    if ((step & 1) == 0 || swing_pct <= 50) return 0;
    if (swing_pct > 75) swing_pct = 75;
    return (uint32_t)(((uint64_t)tick_period_us *
                       (uint32_t)(swing_pct - 50)) /
                      50u);
}

// `next_step` is the step that the following clock will play. If that clock
// is already a bar boundary, it is still safe to install a queued pattern
// before it fires; otherwise wait for the following 16-step boundary.
static inline int drum_next_bar_boundary(int next_step) {
    next_step &= 31;
    if (next_step == 0 || next_step == 16) return next_step;
    return next_step < 16 ? 16 : 0;
}

// Hat velocity stays at unity through 60% fill. Above that, variability grows
// gently to a maximum 12.5% reduction (about 1.2 dB): enough to animate busy
// hats without making them lurch in and out of the mix.
static inline uint16_t drum_hat_velocity_q8(int fill_percent,
                                             uint32_t random_value) {
    if (fill_percent <= 60) return 256;
    if (fill_percent > 100) fill_percent = 100;
    uint32_t depth = (uint32_t)(((fill_percent - 60) * 32 + 20) / 40);
    // Multiply-high maps all 32 random bits uniformly to 0..depth.
    uint32_t reduction =
        (uint32_t)(((uint64_t)random_value * (depth + 1u)) >> 32);
    return (uint16_t)(256u - reduction);
}
