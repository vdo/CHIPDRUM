#pragma once
#include <stdbool.h>
#include <stdint.h>

// Settings persistence in the last flash sector (survives power cycles).
// The selected mode is saved automatically after it settles; the button
// combo still saves all settings immediately.

void persist_load(void);
bool persist_save(void); // returns false on failure
void persist_mode_changed(uint64_t now);
void persist_task(uint64_t now);
