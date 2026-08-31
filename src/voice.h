// =============================================================================
// PWM square-wave voice engine (3 oscillators)
// =============================================================================
#pragma once
#include <stdbool.h>
#include <stdint.h>

void voice_init_all(void);

// Set target pitch in Hz (approached with the voice's glide time)
void voice_freq(int v, float hz);
// Glide time constant in ms (0 = instant pitch changes)
void voice_glide(int v, float ms);
// Duty cycle 1-99 % while gated (timbre)
void voice_duty(int v, uint8_t pct);
// Open the gate for ms milliseconds (0 = hold until voice_gate(v,false))
void voice_gate_ms(int v, uint32_t ms, uint64_t now);
void voice_gate(int v, bool on);
// Percussive pitch sweep: start at f0, decay exponentially to f1 with tau_ms,
// gated for gate_ms
void voice_sweep(int v, float f0, float f1, float tau_ms, uint32_t gate_ms,
                 uint64_t now);
// Call every main-loop iteration
void voice_update(uint64_t now_us, float dt_s);

// For the UI: current audible frequency (0 if gated off)
float voice_current_freq(int v);
bool voice_is_gated(int v);

// Hand a voice's PWM slice over to the sampler (or take it back). While a
// voice is in sample mode voice_update() leaves its slice alone. Called by
// sampler_enable() - modes should not call this directly.
void voice_set_sample_mode(int v, bool on);
