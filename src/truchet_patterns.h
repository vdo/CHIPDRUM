// Shared Truchets/Grids pattern-bank reader for the two drum modes.
#pragma once

#include <stdint.h>

#define TRUCHET_STEPS 32
#define TRUCHET_INSTRUMENTS 3
#define TRUCHET_BANKS 3
#define TRUCHET_BASE_THRESHOLD 127

// Default to the Electronic bank's 4/4 House node (X=0, Y=50).
#define TRUCHET_DEFAULT_BANK 1
#define TRUCHET_DEFAULT_X 0
#define TRUCHET_DEFAULT_Y 50

// The OG, Electronic and Breakbeat banks all interpolate between 5x5 nodes.
const char *truchet_bank_name(int bank);

// Read one step of CHIPDRUM's 32-step (two-bar) phrase. Each 16-step bar
// samples the corresponding Grids map at its native musical rate.
uint8_t truchet_level(int bank, int x, int y, int instrument, int step);

// Build the stable groove heard with the FILL/DENSITY knob at noon.
void truchet_base_masks(int bank, int x, int y,
                        uint32_t out[TRUCHET_INSTRUMENTS]);

// Probability for a non-core map level to become a fill. It rises above the
// center deadband and is biased toward the end of the 32-step phrase.
float truchet_fill_probability(uint8_t level, int step, float density);

// Decide one live hit from the stable groove, density and a random value in
// [0,1). Below noon core hits are thinned deterministically by their bank
// priority; above noon probabilistic fills are added.
int truchet_should_hit(int bank, int x, int y, int instrument, int step,
                       float density, float random_value);
