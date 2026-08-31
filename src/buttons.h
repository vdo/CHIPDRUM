#pragma once
#include <stdint.h>

// UP/DOWN: select slot - LEFT/RIGHT: adjust - EXTRA1 short: manual tick,
// long: summary - EXTRA2 short: restart bar, long: reset mode (or 2-second
// mode-specific randomize) -
// EXTRA1+EXTRA2: save settings to flash

typedef enum {
    ACT_NONE = 0,
    ACT_MODE_CHANGED, // *arg = new mode index
    ACT_RESTART,
    ACT_RESET,
    ACT_RANDOMIZE,
    ACT_SAVE,
} btn_action_t;

btn_action_t buttons_task(uint64_t now, int *arg);
