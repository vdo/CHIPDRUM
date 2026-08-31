#include "chop.h"

#include <stdio.h>

#include "../voice.h"
#include "mode_util.h"

const uint16_t CHOP_MASKS[NUM_CHOPS] = {
    0xFFFF, // HOLD    - continuous (handled specially)
    0x5555, // 8THS    - every other step
    0xDDDD, // GALLOP  - x.xx per beat
    0x4444, // OFFBEAT - upbeats only
    0x1111, // QUARTER - on the beat
    0x9249, // TRIPLET - every 3rd step across the bar
};

const char *const CHOP_NAMES[NUM_CHOPS] = {"HOLD", "8THS", "GALLOP",
                                           "OFFBEAT", "QUARTER", "TRIPLET"};

void chop_fmt(char *out, int16_t v) {
    sprintf(out, "%s", CHOP_NAMES[v % NUM_CHOPS]);
}

void chop_apply(int pattern, int s, uint64_t now) {
    if (pattern == 0) {
        for (int v = 0; v < 3; v++)
            if (!voice_is_gated(v)) voice_gate(v, true);
    } else if ((CHOP_MASKS[pattern] >> s) & 1u) {
        uint32_t gms = gate_ms();
        for (int v = 0; v < 3; v++) voice_gate_ms(v, gms, now);
    }
}
