// =============================================================================
// Adaptive Schmitt trigger for analog clock/gate detection
// =============================================================================
// Pure logic, no SDK dependencies, so it can be unit-tested on the host.
//
// The trigger level tracks the midpoint of the signal that actually arrives
// instead of assuming a fixed voltage. That matters because the CLK IN jack
// shares the CV2 front end, which may attenuate or bias the incoming signal:
// a 5 V gate can land anywhere from 3.3 V down to a few hundred millivolts.
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t lo, hi;        // running envelope of the input
    uint16_t min_swing;     // ignore anything quieter than this (noise guard)
    uint16_t decay;         // envelope shrink per decay tick
    bool level;             // current trigger state
} edge_t;

void edge_init(edge_t *e, uint16_t min_swing, uint16_t decay);

// Feed one sample. Returns true on a rising edge.
// Pass decay_now = true at a steady slow rate (e.g. every 10 ms) so the
// envelope forgets a source that was unplugged or changed level.
bool edge_feed(edge_t *e, uint16_t sample, bool decay_now);

static inline uint16_t edge_swing(const edge_t *e) {
    return e->hi > e->lo ? (uint16_t)(e->hi - e->lo) : 0;
}
