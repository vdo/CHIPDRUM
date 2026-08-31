// Shared rhythmic gating for the drone family (DRONER / ORGAN / GRINDER)
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define NUM_CHOPS 6

extern const uint16_t CHOP_MASKS[NUM_CHOPS]; // 16-step gate masks
extern const char *const CHOP_NAMES[NUM_CHOPS];

void chop_fmt(char *out, int16_t v);

// Applies the chop pattern for step `s` to all three voices.
// Pattern 0 (HOLD) keeps the gates open continuously.
void chop_apply(int pattern, int s, uint64_t now);
