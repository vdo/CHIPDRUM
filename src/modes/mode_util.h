// Shared helpers for modes
#pragma once
#include <stdint.h>

#include "../clockgen.h"
#include "../scales.h"
#include "../state.h"

// Gate length in ms derived from the tick period and the global GATE param
static inline uint32_t gate_ms(void) {
    uint32_t ms = clock_period_us() / 1000u * (uint32_t)g.gate_pct / 100u;
    if (ms < 5) ms = 5;
    if (ms > 2000) ms = 2000;
    return ms;
}

// Frequency of the bass root at the current ROOT/OCT settings
static inline float root_freq(void) { return midi_to_freq((float)base_midi()); }

static inline float semi_freq(int offset) {
    return midi_to_freq((float)(base_midi() + offset));
}
