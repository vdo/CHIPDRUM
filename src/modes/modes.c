// =============================================================================
// Mode registry + global parameter slots
// =============================================================================
#include <stdio.h>

#include "../mode.h"
#include "../state.h"

extern const mode_t MODE_BASSLOOP;
extern const mode_t MODE_DRONER;
extern const mode_t MODE_ORGAN;
extern const mode_t MODE_GRINDER;
extern const mode_t MODE_DRUMS;
extern const mode_t MODE_SYNDRUM;

const mode_t *const MODES[] = {
    // Rhythmic modes first. Slot 0 is also the clean-boot default.
    &MODE_DRUMS,
    &MODE_SYNDRUM,
    &MODE_BASSLOOP,
    // Drone-style modes stay together at the end of the menu.
    &MODE_DRONER,
    &MODE_ORGAN,
    &MODE_GRINDER,
};
const int NUM_MODES = sizeof(MODES) / sizeof(MODES[0]);

static void fmt_root(char *out, int16_t v) {
    static const char *const N[12] = {"C",  "C#", "D",  "D#", "E",  "F",
                                      "F#", "G",  "G#", "A",  "A#", "B"};
    sprintf(out, "%s", N[v % 12]);
}

static void fmt_oct(char *out, int16_t v) { sprintf(out, "C%d", v + 1); }

static void fmt_pct(char *out, int16_t v) { sprintf(out, "%d%%", v); }

const param_t GLOBAL_PARAMS[NUM_GLOBAL_PARAMS] = {
    {"ROOT", &g.root, 0, 11, 1, fmt_root},
    {"OCT", &g.octave, 0, 3, 1, fmt_oct},
    {"GATE", &g.gate_pct, 10, 90, 5, fmt_pct},
};

const param_t *param_for_slot(int slot) {
    const mode_t *m = MODES[g.mode_idx];
    int globals = m->hide_globals ? 0 : NUM_GLOBAL_PARAMS;
    if (slot >= 1 && slot <= globals) return &GLOBAL_PARAMS[slot - 1];
    int mi = slot - 1 - globals;
    if (mi >= 0 && mi < m->n_params) return &m->params[mi];
    return 0;
}

int num_slots(void) {
    const mode_t *m = MODES[g.mode_idx];
    return 1 + (m->hide_globals ? 0 : NUM_GLOBAL_PARAMS) + m->n_params;
}
