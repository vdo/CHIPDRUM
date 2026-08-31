// =============================================================================
// Mode interface + editable parameters
// =============================================================================
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const char *name;                      // short, uppercase
    int16_t *value;                        // points at the mode's variable
    int16_t min, max, step;
    void (*fmt)(char *out, int16_t v);     // NULL -> plain number
} param_t;

typedef struct {
    const char *name; // short uppercase name for the header
    void (*reset)(void);
    // Called once per clock tick (16th note). now = time_us_64().
    void (*on_tick)(uint64_t now);
    // Called every main-loop iteration (glide/LFO/etc). May be NULL.
    void (*update)(float dt_s, uint64_t now);
    // Called from CORE 1 at ~15 FPS. Must only read mode state - tearing is
    // cosmetic, but never write shared state from here.
    void (*draw)(void);
    // Optional short string for the header, e.g. a live FILL readout. The UI
    // places it between the mode name and the clock readout and drops it if
    // there is no room, so a mode can never overprint the header itself.
    // Keep it to about 5 characters.
    void (*header)(char *out);
    // Optional phase restart for a short STOP press. If absent, reset() is
    // used. Drum modes preserve normal playback but return to step 0.
    void (*restart)(void);
    // Optional action for a 2-second STOP hold. Drum modes use it to
    // randomize their X/Y coordinates; modes without it retain long-reset.
    void (*randomize)(int16_t *x, int16_t *y);
    const param_t *params;
    int n_params;
    // Drum modes ignore ROOT/OCT/GATE, so they hide those slots rather than
    // making you scroll past controls that do nothing.
    bool hide_globals;
} mode_t;

extern const mode_t *const MODES[];
extern const int NUM_MODES;

// Slot layout: 0=MODE, 1=ROOT, 2=OCT, 3=GATE, then the mode's own params.
#define NUM_GLOBAL_PARAMS 3
extern const param_t GLOBAL_PARAMS[NUM_GLOBAL_PARAMS];
// Returns the param for slot >= 1 (NULL if out of range for the active mode)
const param_t *param_for_slot(int slot);
int num_slots(void); // 1 + globals + active mode params
