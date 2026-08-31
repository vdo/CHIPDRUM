// =============================================================================
// PWM sample playback engine
// =============================================================================
// Each voice can be switched from square-oscillator mode into sample mode.
// In sample mode its PWM slice runs as a DAC: each unsigned 8-bit source
// sample is scaled over DAC_STEPS PWM levels. A timer interrupt advances all
// three voices at SAMPLE_RATE_HZ.
//
// An RC filter on the output (1k + 10nF) reconstructs the audio cleanly; the
// carrier is far enough above audio that it is inaudible either way.
#pragma once
#include <stdbool.h>
#include <stdint.h>

void sampler_init(void);

// Put a voice into sample mode (reconfigures its PWM slice) or back to
// square-oscillator mode.
void sampler_enable(int voice, bool on);
bool sampler_enabled(int voice);

// Trigger a one-shot. semitones shifts playback rate (0 = native pitch).
// velocity_q8 is 0..256, with 256 = the sample's original level.
void sampler_trigger(int voice, const uint8_t *pcm, uint32_t len,
                     int semitones, uint16_t velocity_q8);

// Switch a voice between PCM playback and live drum synthesis. Both use the
// same 8-bit DAC mode, so a voice must already be sampler_enable()d.
void sampler_set_synth(int voice, bool on);

bool sampler_playing(int voice);
// 0..255 progress through the current sample, for the display
uint8_t sampler_progress(int voice);
