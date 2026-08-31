// Small fast PRNG (xorshift32) - deterministic, no libc rand() state
#pragma once
#include <stdint.h>

extern uint32_t g_rng_state;

static inline uint32_t rng_u32(void) {
    uint32_t x = g_rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g_rng_state = x;
    return x;
}

// uniform float in [0, 1)
static inline float rng_f(void) {
    return (rng_u32() >> 8) * (1.0f / 16777216.0f);
}

// uniform int in [0, n)
static inline uint32_t rng_below(uint32_t n) { return rng_u32() % n; }

static inline uint32_t rng_bit(void) { return rng_u32() & 1u; }
