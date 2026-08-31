// =============================================================================
// Clock engine: internal BPM clock, external clock in (GPIO IRQ on GP27),
// divide/multiply via the slider, master clock out on GP1.
// One tick = one 16th note.
// =============================================================================
#pragma once
#include <stdbool.h>
#include <stdint.h>

void clock_init(void);

// Call every main-loop iteration. slider_norm 0..1. Returns the number of
// ticks to fire now (0..4). Handles the clock-out pulse timing internally
// (a pulse is emitted for every fired tick).
int clock_task(uint64_t now, float slider_norm);

// Queue one extra tick (manual tap); also emits a clock-out pulse.
void clock_manual_tick(void);

bool clock_is_external(void);
// Effective BPM (for display)
float clock_bpm(void);
// Current divide/multiply factor: negative = divide (÷n), positive = multiply
// (×n), 1 = 1:1. Only meaningful with external clock.
int clock_factor(void);
// Effective tick period in microseconds (for gate lengths)
uint32_t clock_period_us(void);

// --- External clock input diagnostics (shown on the status screen) ---
float clock_in_volts(void);       // live voltage at the CLK IN jack
float clock_in_swing_volts(void); // detected peak-to-peak swing
bool clock_in_level(void);        // current state of the adaptive trigger
