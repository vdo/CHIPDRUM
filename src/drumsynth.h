// =============================================================================
// Real-time drum synthesis (909-flavoured, industrial snare)
// =============================================================================
// Runs inside the 25 kHz audio interrupt, so everything here is fixed-point:
// the Cortex-M0+ has no FPU and software floats cost ~100 cycles an operation.
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum { DRUM_KICK = 0, DRUM_SNARE, DRUM_HAT } drum_t;

void drumsynth_init(void);

// tune: semitones (-24..+24). decay: 0..100, mapped per drum to a musically
// useful range. Both are read at trigger time.
void drumsynth_trigger(int voice, drum_t drum, int tune, int decay);
// As above, with Q8 velocity (256 = unity).
void drumsynth_trigger_velocity(int voice, drum_t drum, int tune, int decay,
                                uint16_t velocity_q8);

// Render one sample for a voice. Returns -128..127. ISR context.
int drumsynth_tick(int voice);

bool drumsynth_active(int voice);
