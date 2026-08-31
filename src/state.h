// =============================================================================
// TECLA BASS - global state shared between cores
// =============================================================================
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    volatile int16_t mode_idx; // active mode (core1 reads this)
    int16_t param_idx;         // selected UI slot (0 = MODE)
    bool show_summary;         // EXTRA1 held long

    // Global musical parameters (editable slots on every mode)
    int16_t root;     // 0-11 semitones above C
    int16_t octave;   // 0-3 -> base note C1..C4
    int16_t gate_pct; // gate length, % of tick period (10-90)

    // Smoothed analog inputs, 0..1
    float cv1;
    float slider;

    uint32_t step; // global tick counter

    // Toast message (e.g. "SAVED"), shown until toast_until
    char toast[12];
    char toast_detail[12];
    uint64_t toast_until;
} state_t;

extern state_t g;

// Base MIDI note of the bass voice: C1 = MIDI 24
static inline int base_midi(void) { return 24 + 12 * g.octave + g.root; }
