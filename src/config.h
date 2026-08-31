// =============================================================================
// TECLA BASS - hardware configuration
// =============================================================================
#pragma once

// --- PWM oscillator outputs (square-wave voices) ---
#define PIN_VOICE1 22 // bass          (PWM slice 3, channel A)
#define PIN_VOICE2 2  // sub-octave    (PWM slice 1, channel A)
#define PIN_VOICE3 0  // accent/fifth  (PWM slice 0, channel A)
#define NUM_VOICES 3

// --- Clock I/O ---
#define PIN_CLK_OUT 1  // master clock output jack (was "gate")
#define PIN_CLK_IN 27  // external clock input (old CV2 jack, schmitt pad)

// --- Analog inputs ---
#define ADC_CV1 0    // GP26: macro CV / probability / chaos amount
#define ADC_CLK_IN 1 // GP27: external clock (read as ANALOG - see clockgen.c)
#define ADC_SLIDER 2 // GP28: tempo / clock divide-multiply
#define PIN_ADC_CV1 26
#define PIN_ADC_SLIDER 28

// --- Buttons (active high, pull-down) ---
#define PIN_BTN_UP 13
#define PIN_BTN_DOWN 14
#define PIN_BTN_LEFT 15
#define PIN_BTN_RIGHT 3
#define PIN_BTN_EXTRA1 5
#define PIN_BTN_EXTRA2 4
#define NUM_BUTTONS 6

// --- LEDs ---
// LED1 = external clock present, LED2 = clock pulse, LED3-7 = param slot
#define PIN_LED_1 10
#define PIN_LED_2 6
#define PIN_LED_3 8
#define PIN_LED_4 9
#define PIN_LED_5 7
#define PIN_LED_6 11
#define PIN_LED_7 12
#define NUM_LEDS 7

// --- OLED (SSD1306 128x64) ---
#define OLED_I2C i2c0
#define PIN_OLED_SDA 20
#define PIN_OLED_SCL 21
#define OLED_I2C_HZ 1000000 // drop to 400000 if your display is unstable
#define OLED_ADDR 0x3C

// --- Clock behaviour ---
// One tick = one 16th note. Clock out emits one pulse per tick.
// The slider maps to tempo in two straight segments meeting at BPM_CENTER, so
// 120 BPM sits exactly at the middle of the throw. A single linear sweep would
// put 135 there and leave the useful slow end cramped.
#define BPM_MIN 30
#define BPM_CENTER 120
#define BPM_MAX 240
// A slider's electrical midpoint rarely lands on exactly half scale - this one
// reads about 0.53 at its mechanical centre, which mapped to 127 BPM. Rather
// than trim an offset that is specific to one unit, snap a zone around the
// middle to BPM_CENTER: robust to any small offset, and it gives the most
// useful tempo a detent you can find by feel, like the centre notch on a pan
// pot. Widen this if your slider sits further off centre.
#define SLIDER_DETENT 0.06f
#define CLOCK_PULSE_US 5000        // clock-out pulse width
#define EXT_TIMEOUT_US 2000000     // no edges for 2 s -> back to internal clock
#define EXT_PERIOD_MIN_US 5000     // ignore implausible periods
#define EXT_PERIOD_MAX_US 4000000

// External clock detection.
// GP27 is read with the ADC, not as a digital pin: the jack's front end may
// attenuate or bias the signal (it is shared with the CV2/LDR circuit), so an
// incoming clock can easily stay below the RP2040's ~2.1 V logic threshold.
// The trigger level adapts to whatever swing actually arrives.
#define CLK_MIN_SWING 250      // ~0.2 V p-p required before we trust a signal
#define CLK_ENVELOPE_DECAY 6   // counts per decay tick, re-adapts in ~2 s
#define CLK_DECAY_INTERVAL_US 10000

// --- Audio engine (sampler + drum synth) ---
// The PWM DAC period and the sample clock are locked to an exact integer
// ratio: 250 counts at 125 MHz is exactly 2 us, and the 44 us sample period
// is exactly 22 of them. That matters because the duty register is written
// from the audio interrupt while the PWM is running - with a non-integer
// ratio the write lands at a drifting point in the PWM cycle and occasionally
// produces a malformed pulse. Those glitches are inaudible under noise, but a
// sustained pure tone (a long-decay kick) puts a whine on top of them.
// Resolution matters more than it looks: a drum synth's kick is a pure sine,
// the worst case for quantization noise, and 8 bits leaves it audibly grainy.
// 1250 duty steps is ~10.3 bits, a 12 dB improvement, and 1250 counts at
// 125 MHz is exactly 10 us - four of them per 40 us sample.
#define AUDIO_SR 25000    // 1e6 / 40 us
#define AUDIO_PERIOD_US 40
#define DAC_WRAP 1249     // 1250 counts -> 100 kHz carrier, exactly 10 us
#define DAC_STEPS 1250
#define SYNTH_FULL 2047   // drumsynth works at 12 bits internally

// 8-bit PCM sample (0..255) -> duty
#define DAC_FROM_PCM8(v) ((uint16_t)(((uint32_t)(v) * DAC_STEPS) >> 8))
// signed 12-bit synth sample -> duty
#define DAC_FROM_SYNTH(v) ((uint16_t)((((int32_t)(v) + 2048) * DAC_STEPS) >> 12))

// --- Voices ---
// clkdiv 128 -> 976.5625 kHz counter. The 16-bit wrap then reaches down to
// 976562.5/65536 = 14.9 Hz, so the sub-octave of the lowest root (C0, 16.35 Hz)
// plays at its true pitch instead of being silently clamped sharp.
#define PWM_CLKDIV 128.0f
#define PWM_COUNT_HZ 976562.5f
#define VOICE_FREQ_MIN 15.0f
#define VOICE_FREQ_MAX 8000.0f
