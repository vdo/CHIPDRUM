#include "buttons.h"

#include <stdbool.h>

#include "clockgen.h"
#include "config.h"
#include "hardware.h"
#include "mode.h"
#include "state.h"

#define HOLD_LONG_US 1500000
#define HOLD_RANDOM_US 2000000
#define REPEAT_DELAY_US 400000
#define REPEAT_RATE_US 120000

static bool prev[NUM_BUTTONS];
static uint64_t t_press[NUM_BUTTONS];
static bool long_done[NUM_BUTTONS];
static uint64_t t_repeat[NUM_BUTTONS];
static bool combo_done;

// UP DOWN LEFT RIGHT E1 E2
enum { B_UP, B_DOWN, B_LEFT, B_RIGHT, B_E1, B_E2 };

static btn_action_t adjust(int delta, int *arg) {
    if (g.param_idx == 0) {
        int m = (g.mode_idx + delta + NUM_MODES) % NUM_MODES;
        *arg = m;
        return ACT_MODE_CHANGED;
    }
    const param_t *p = param_for_slot(g.param_idx);
    if (p) {
        int v = *p->value + delta * p->step;
        if (v < p->min) v = p->min;
        if (v > p->max) v = p->max;
        *p->value = (int16_t)v;
    }
    return ACT_NONE;
}

btn_action_t buttons_task(uint64_t now, int *arg) {
    bool vals[NUM_BUTTONS];
    for (int i = 0; i < NUM_BUTTONS; i++) vals[i] = button_read(i);
    btn_action_t action = ACT_NONE;

    // --- combo EXTRA1+EXTRA2: save settings ---
    if (vals[B_E1] && vals[B_E2]) {
        if (!combo_done) {
            combo_done = true;
            long_done[B_E1] = true;
            long_done[B_E2] = true;
            action = ACT_SAVE;
        }
        goto out;
    }
    if (!vals[B_E1] && !vals[B_E2]) combo_done = false;

    int nslots = num_slots();
    if (g.param_idx >= nslots) g.param_idx = 0;

    // --- UP/DOWN: slot selection ---
    if (vals[B_UP] && !prev[B_UP])
        g.param_idx = (int16_t)((g.param_idx + nslots - 1) % nslots);
    if (vals[B_DOWN] && !prev[B_DOWN])
        g.param_idx = (int16_t)((g.param_idx + 1) % nslots);
    if ((vals[B_UP] && !prev[B_UP]) || (vals[B_DOWN] && !prev[B_DOWN]))
        led_show_param_slot(g.param_idx);

    // --- LEFT/RIGHT: adjust with accelerating repeat ---
    static const int DELTAS[2] = {-1, 1};
    for (int j = 0; j < 2; j++) {
        int i = j == 0 ? B_LEFT : B_RIGHT;
        if (vals[i] && !prev[i]) {
            t_press[i] = now;
            t_repeat[i] = now + REPEAT_DELAY_US;
            btn_action_t a = adjust(DELTAS[j], arg);
            if (a != ACT_NONE) action = a;
        } else if (vals[i] && now >= t_repeat[i]) {
            uint64_t held = now - t_press[i];
            int accel = held > 2000000 ? 4 : (held > 800000 ? 2 : 1);
            t_repeat[i] = now + REPEAT_RATE_US / (uint64_t)accel;
            if (g.param_idx != 0) adjust(DELTAS[j], arg); // no auto mode change
        }
    }

    // --- EXTRA1: short = manual tick, long = summary ---
    if (vals[B_E1] && !prev[B_E1]) {
        t_press[B_E1] = now;
        long_done[B_E1] = false;
    } else if (!vals[B_E1] && prev[B_E1]) {
        if (!long_done[B_E1] && (now - t_press[B_E1]) < HOLD_LONG_US)
            clock_manual_tick();
    }
    g.show_summary =
        vals[B_E1] && !long_done[B_E1] && (now - t_press[B_E1]) > HOLD_LONG_US;

    // --- EXTRA2: short = restart bar, long = reset or mode randomize ---
    if (vals[B_E2] && !prev[B_E2]) {
        t_press[B_E2] = now;
        long_done[B_E2] = false;
    } else if (vals[B_E2] && !long_done[B_E2] &&
               (now - t_press[B_E2]) >
                   (MODES[g.mode_idx]->randomize ? HOLD_RANDOM_US
                                                 : HOLD_LONG_US)) {
        long_done[B_E2] = true;
        action = MODES[g.mode_idx]->randomize ? ACT_RANDOMIZE : ACT_RESET;
    } else if (!vals[B_E2] && prev[B_E2]) {
        if (!long_done[B_E2]) action = ACT_RESTART;
    }

out:
    for (int i = 0; i < NUM_BUTTONS; i++) prev[i] = vals[i];
    return action;
}
