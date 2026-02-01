# TECLA - Complete User Manual

**TECLA** is a professional-grade hardware synthesizer and generative music instrument designed for the Raspberry Pi Pico (RP2040). It combines mathematical algorithms, real-time PWM synthesis, and interactive controls to create dynamic, evolving musical compositions in the chiptune aesthetic.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Hardware Setup](#2-hardware-setup)
3. [Controls & Interface](#3-controls--interface)
4. [Musical Modes (0-14)](#4-musical-modes-0-14)
5. [Audio Synthesis](#5-audio-synthesis)
6. [Parameter Configuration](#6-parameter-configuration)
7. [CV Calibration](#7-cv-calibration)
8. [Display & Visual Feedback](#8-display--visual-feedback)
9. [External Clock Input](#9-external-clock-input)
10. [MIDI Integration](#10-midi-integration)
11. [Quick Reference](#11-quick-reference)
12. [Troubleshooting](#12-troubleshooting)
13. [Technical Specifications](#13-technical-specifications)

---

## 1. Overview

TECLA transforms your Raspberry Pi Pico into a standalone musical instrument capable of generating complex, evolving melodies and rhythms. At its core, the system features:

- **15 distinct musical modes** (0-14), each with unique generative algorithms
- **3-voice PWM synthesis** with independent duty cycle and harmonic control
- **Real-time CV control** via potentiometer and light-dependent resistor (LDR)
- **BPM slider control** ranging from 20 to 220 beats per minute
- **OLED display** with mode visualization and parameter readouts
- **7 LED indicators** for mode identification and gate feedback
- **USB MIDI output** for DAW integration
- **Chaos mode** for experimental, unpredictable compositions

### Design Philosophy

TECLA is built on the principle that musical complexity can emerge from simple mathematical rules. Each mode implements a different algorithmic approach to note generation—from fractal mathematics to Euclidean rhythms, from modal scales to counterpoint voices. The CV inputs allow real-time modulation of these algorithms, making every performance unique.

---

## 2. Hardware Setup

### Pin Assignments

| Function | GPIO Pin | Description |
|----------|----------|-------------|
| **PWM1** | GP22 | Main audio output channel |
| **PWM2** | GP2 | Harmonic channel 1 |
| **PWM3** | GP0 | Harmonic channel 2 |
| **Gate Out** | GP1 | Digital gate/trigger output (3.3V TTL) |
| **Slider** | GP28 | Parameter 1 control (analog input) |
| **CV1/Pot** | GP26 | Tempo control (pot) OR Clock input (jack) |
| **CV2/LDR** | GP27 | Parameter 2 control (analog input) |
| **D-pad Up** | GP13 | Octave up / Chaos toggle |
| **D-pad Down** | GP14 | Octave down / Chaos toggle |
| **D-pad Left** | GP15 | Previous mode / Decrease parameter |
| **D-pad Right** | GP3 | Next mode / Increase parameter |
| **Extra 1** | GP5 | Config cycle / Summary |
| **Extra 2** | GP4 | Reverse cycle / Pause |
| **LED 1** | GP10 | Mode indicator |
| **LED 2** | GP6 | Gate activity indicator |
| **LED 3** | GP8 | Mode indicator |
| **LED 4** | GP9 | Config indicator |
| **LED 5** | GP7 | Config indicator |
| **LED 6** | GP11 | Config indicator |
| **LED 7** | GP12 | Mode indicator |
| **Display SDA** | GP20 | I2C data for OLED |
| **Display SCL** | GP21 | I2C clock for OLED |

### Audio Output

The three PWM outputs can be connected directly to a mixer, amplifier, or headphones (with appropriate impedance matching). For best results:

- Use a simple RC low-pass filter (10kΩ resistor + 100nF capacitor) to smooth the PWM signal
- Combine PWM1, PWM2, and PWM3 through a passive mixer for polyphonic output
- The gate output (GP1) provides a 3.3V pulse synchronized with note-on events

---

## 3. Controls & Interface

### Physical Layout

```
              [▲] Octave Up (GP13)
               │
     Left [◄]─[●]─[►] Right  D-Pad
     GP15      │      GP3
              [▼] Octave Down (GP14)

[Slider] ─────────────── Parameter 1
[CV1 Pot/Jack] ─────────Tempo (pot) / Clock Input (jack)
[CV2 LDR] ──────────────Parameter 2

[Extra 1] ──────────────Config / Summary
[Extra 2] ──────────────Reverse / Pause
```

### Button Functions

#### D-Pad Navigation

| Button | Normal Mode | Config Mode |
|--------|-------------|-------------|
| **▲ Up** | Increase octave (0→8) | — |
| **▼ Down** | Decrease octave (8→0) | — |
| **◄ Left** | Previous mode (wraps 1→14) | Decrease current parameter |
| **► Right** | Next mode (wraps 14→1) | Increase current parameter |

**Note:** Mode 0 (Pausa) is only accessible via long-press on Extra 2. The D-pad cycles through modes 1-14.

**Octave Limits & Chaos Mode:**
- When at octave 8, pressing Up toggles Chaos mode on
- When at octave 0, pressing Down toggles Chaos mode on
- Chaos mode adds random octave transposition to every note

#### Extra Buttons

| Button | Short Press | Long Press (1.5s) |
|--------|-------------|-------------------|
| **Extra 1** | Cycle to next config parameter | Show full summary display |
| **Extra 2** | Cycle to previous config parameter | Pause (enter Mode 0) |

**Combined Action:**
- Hold **Extra 1 + Extra 2** together for 0.5 seconds to enter CV Calibration mode

### Parameter Acceleration

When adjusting parameters with the D-pad in config mode, holding the button causes acceleration:

| Hold Duration | Speed Multiplier |
|---------------|------------------|
| 0 - 0.8s | 1x |
| 0.8 - 1.5s | 2x |
| 1.5 - 2.0s | 4x |
| > 2.0s | 8x |

This allows quick traversal of large value ranges while maintaining precision for fine adjustments.

---

## 4. Musical Modes (0-14)

Each mode implements a unique algorithm for generating musical notes. The CV1 and CV2 inputs modulate mode-specific parameters.

### Mode 0: PAUSA (Pause)

**Purpose:** Silent idle state with visual animation

- **Audio:** All PWM channels off, no MIDI output
- **Display:** Random pixel animation ("dau" effect)
- **LEDs:** All LEDs change randomly every 0.5 seconds
- **Use Case:** Pause between compositions, visual standby

---

### Mode 1: FRACTAL (Mandelbrot)

**Algorithm:** Iterative Mandelbrot set calculation

**Controls:**
- **CV1:** X coordinate in fractal space (-1.5 to +1.5)
- **CV2:** Y coordinate in fractal space (-1.5 to +1.5)
- **Slider:** Iteration speed (BPM)

**Musical Character:** Mathematical, alien, otherworldly

**How It Works:**
The Mandelbrot set is calculated at coordinates determined by CV1 and CV2. The iteration count before escape determines the MIDI note value. Moving through different regions of the fractal creates melodic variation—boundary regions produce constantly changing notes while deep-set regions produce stable tones.

**Chaos Mode Effect:** Random octave jumps create polyrhythmic chaos

---

### Mode 2: RIU (River)

**Algorithm:** Sine wave + cosine ripple with random offset

**Controls:**
- **CV1:** Density (1-10 semitone steps per beat)
- **CV2:** Turbulence/wave amplitude (0-127)

**Gate Pattern:** `[1,1,1,0,0,1,1,1,1,0]` (repeating)

**Musical Character:** Flowing, organic, liquid

**How It Works:**
Notes flow like water—rising and falling in sine wave patterns with small random variations creating natural-sounding motion. The gate pattern creates a 7-of-10 rhythmic feel.

**Chaos Mode Effect:** Notes scatter across random octaves, creating echo effects

---

### Mode 3: TEMPESTA (Storm)

**Algorithm:** Rain baseline with random lightning bursts

**Controls:**
- **CV1:** Rain intensity (base note offset, 0-127)
- **CV2:** Lightning frequency (0-100% trigger probability)

**Musical Character:** Dramatic, dynamic, percussive

**How It Works:**
- **Rain Phase:** Small random variations (±3 semitones) on the base note
- **Lightning Phase:** Ascending/descending arpeggio bursts over scale [0,3,5,7,10]
- **Direction:** 70% ascending, 30% descending

**Chaos Mode Effect:** Lightning strikes in random octaves

---

### Mode 4: HARMONIA (Harmony)

**Algorithm:** Chord progression generator

**Controls:**
- **CV1:** Root note selection (0-6 from major scale)
- **CV2:** Chord type tension:
  - 0: Major triad [0,4,7]
  - 1: Minor triad [0,3,7]
  - 2: Major 7th [0,4,7,11]
  - 3: Minor 7th [0,3,7,10]

**Musical Character:** Harmonic, sophisticated, jazz-like

**How It Works:**
Cycles through chord tones sequentially, creating arpeggiated harmonic progressions. The CV inputs control both the root and the chord quality, allowing real-time harmonic exploration.

**Chaos Mode Effect:** Chord notes scattered across random octaves

---

### Mode 5: BOSC (Forest)

**Algorithm:** Random note selection with controlled jumps

**Controls:**
- **CV1:** Density (1-10, affects interval jump size)
- **CV2:** Depth (0-7 octave range span)

**Interval Jumps:** Random selection from [-2, -1, 0, 1, 2, 4, 7] semitones

**Gate Probability:** 66% chance of playing vs. silence

**Musical Character:** Organic, unpredictable, bird-like

**How It Works:**
Notes appear like sounds in a forest—random intervals, occasional silences, with the CV controlling how densely packed and how wide-ranging the notes are.

**Chaos Mode Effect:** Random octave transposition per note

---

### Mode 6: ESCALA CV (Modal Scale)

**Algorithm:** Quantizer to 7 Gregorian modes

**Controls:**
- **CV1:** Mode selection (0-6)
- **CV2:** Scale step speed (1-32 steps per beat)

**Available Modes:**

| Index | Mode | Scale Degrees | Character |
|-------|------|---------------|-----------|
| 0 | Ionian (Major) | [0,2,4,5,7,9,11] | Happy, resolved |
| 1 | Dorian | [2,4,6,7,9,11,1] | Minor, jazzy |
| 2 | Phrygian | [4,6,8,9,11,1,3] | Dark, flamenco |
| 3 | Lydian | [5,7,9,10,0,2,4] | Bright, whimsical |
| 4 | Mixolydian | [7,9,11,0,2,4,6] | Dominant, blues |
| 5 | Aeolian (Minor) | [9,11,1,2,4,6,8] | Melancholic |
| 6 | Locrian | [11,1,3,4,6,8,10] | Dark, diminished |

**Musical Character:** Melodic, consonant, always musical

**Key Feature:** Notes are always quantized to the selected scale, ensuring consonant output regardless of input.

**Chaos Mode:** Disabled (always plays in tune)

---

### Mode 7: EUCLIDIA (Euclidean Rhythm)

**Algorithm:** Euclidean rhythm distribution (Toussaint's algorithm)

**Controls:**
- **CV1:** Pulses (1-32 active beats distributed across 32 steps)
- **CV2:** Accents (1-32 accent points)

**Musical Character:** Rhythmic, mathematical, polyrhythmic

**How It Works:**
Euclidean rhythms distribute N pulses as evenly as possible across M steps. Classic examples:
- 3 pulses in 8 steps = [1,0,0,1,0,0,1,0] (Cuban tresillo)
- 5 pulses in 8 steps = [1,0,1,1,0,1,1,0] (Cuban cinquillo)

The accent pattern adds a second layer of rhythmic emphasis.

**Chaos Mode Effect:** Random octave for each note

---

### Mode 8: COSMOS (Cosmic Synthesis)

**Algorithm:** Hybrid of Fractal + Sinusoidal + Harmonic + Euclidean

**Components:**
1. Mandelbrot coordinates → MIDI note
2. Sinusoidal modulation with CV-controlled frequency/amplitude
3. Harmonic interval generator
4. Euclidean rhythm pattern for gating

**Final Note:** Average of all components with controlled variation

**Musical Character:** Complex, evolving, maximalist

**How It Works:**
All four algorithmic approaches combine to create richly layered, constantly evolving textures. The Euclidean rhythm determines when notes sound.

**Chaos Mode Effect:** ±N random offset applied to final note

---

### Mode 9: CAMPANETES (Bells)

**Algorithm:** Bell simulation with major chord arpeggios

**Controls:**
- **CV1:** Density (0-100% probability of playing each beat)
- **CV2:** Brightness (10-100% gate duration)

**Bell Notes:** Do-Mi-Sol arpeggio [0, 4, 7] (major triad)

**Progression:** Every 3 notes, transpose up by minor third (3 semitones)

**Musical Character:** Orchestral, resonant, meditative

**How It Works:**
Simulates the harmonic richness of bells, with notes ringing in major-third intervals. The brightness control affects sustain length.

---

### Mode 10: SEGONES (Seconds)

**Algorithm:** Two parallel voices with variable separation

**Controls:**

**CV1 (Velocity/Direction):**
| Range | Movement |
|-------|----------|
| 0.00 - 0.15 | -6 semitones/beat (fast down) |
| 0.15 - 0.30 | -4 semitones/beat |
| 0.30 - 0.45 | -2 semitones/beat |
| 0.45 - 0.55 | Hold (dead zone) |
| 0.55 - 0.70 | +2 semitones/beat |
| 0.70 - 0.85 | +4 semitones/beat |
| 0.85 - 1.00 | +6 semitones/beat (fast up) |

**CV2 (Harmonic Separation):**
| Range | Interval |
|-------|----------|
| 0.00 - 0.15 | 1-2 semitones (seconds) |
| 0.15 - 0.30 | 3-4 semitones (thirds) |
| 0.30 - 0.45 | 5-6 semitones (fourths/tritone) |
| 0.45 - 0.55 | 7 semitones (perfect fifth) |
| 0.55 - 0.70 | 8-9 semitones (sixths) |
| 0.70 - 0.85 | 10-11 semitones (sevenths) |
| 0.85 - 1.00 | 12 semitones (octave) |

**Output:**
- **PWM1:** Base note in evolution
- **PWM2:** Base note + harmonic separation

**Musical Character:** Duet, melodic counterpoint, Renaissance-inspired

---

### Mode 11: ESPIRAL (Spiral)

**Algorithm:** Scale traversal with continuous transposition

**Controls:**
- **CV1:** Advance speed (1-7 scale steps per iteration)
- **CV2:** Transposition cycle (1-32 beats between semitone rises)

**Scale:** Major scale [0, 2, 4, 5, 7, 9, 11, 12]

**Musical Character:** Uplifting, transformative, gradient-like

**How It Works:**
Walks through scale degrees while periodically transposing upward by a semitone. The result is a gradually rising spiral through pitch space.

**Chaos Mode Effect:** Random octave jumps

---

### Mode 12: CONTRAPUNT (Counterpoint)

**Algorithm:** Three independent melodic voices

**Controls:**
- **CV1:** PWM2 density (1-8 beat interval for consonant third voice)
- **CV2:** PWM3 density (1-8 beat interval for dissonant tritone voice)

**Voices:**
- **PWM1 (always):** Major scale degree
- **PWM2 (CV1-controlled):** Major third above (consonant)
- **PWM3 (CV2-controlled):** Tritone above (dissonant)

**Density Inversion:** Low CV = sparse, high CV = dense

**Musical Character:** Baroque, layered, harmonic tension/release

---

### Mode 13: NARVAL (Narwhals)

**Algorithm:** Three "narwhal voices" with call-and-response

**Controls:**
- **CV1:** Call probability (0-100% how often narwhals "speak")
- **CV2:** Response mood:
  - 0.00 - 0.33: Sad [3, 6, 10] (minor intervals)
  - 0.33 - 0.66: Neutral [5, 7, 12] (perfect intervals)
  - 0.66 - 1.00: Happy [4, 9, 12] (major intervals)

**Scale:** Pentatonic [0, 2, 4, 7, 9]

**Dynamics:**
- 0 narwhals: Silence
- 1 narwhal calls: Others respond with 80% probability
- 2 narwhals: Third dances with 50% response rate
- 3 narwhals: Full harmony with response intervals

**Musical Character:** Whimsical, social, generative storytelling

**Chaos Mode Effect:** Random octave for all voices

---

### Mode 14: CICLADOR (Oscillator)

**Algorithm:** Fixed note with duty cycle exploration

**Controls:**
- **CV1:** PWM1 duty cycle (0-99%)
- **CV2:** PWM2 duty cycle (0-99%)
- **Slider:** PWM3 duty cycle (0-99%)

**Fixed Note:** C4 (MIDI 60), transposable via octave controls

**Duty Cycle Effects:**
- 1% = Narrow pulse (buzzy, harmonic-rich)
- 50% = Square wave (hollow, classic chiptune)
- 99% = Near-sine (smooth, pure)

**Musical Character:** Textural, educational, synthesis exploration

**Chaos Mode:** Disabled (note stays fixed)

---

## 5. Audio Synthesis

### PWM Synthesis Engine

TECLA uses Pulse Width Modulation (PWM) to generate audio. Three independent PWM channels provide polyphonic capability:

| Channel | GPIO | Function |
|---------|------|----------|
| PWM1 | GP22 | Main melody voice |
| PWM2 | GP2 | Harmonic voice 1 |
| PWM3 | GP0 | Harmonic voice 2 |

### Frequency Range

| Octave | Base Note | Frequency | MIDI Note |
|--------|-----------|-----------|-----------|
| 0 | C0 | 8.18 Hz | 0 |
| 1 | C1 | 16.35 Hz | 12 |
| 2 | C2 | 32.70 Hz | 24 |
| 3 | C3 | 65.41 Hz | 36 |
| 4 | C4 | 130.81 Hz | 48 |
| 5 | C5 | 261.63 Hz | 60 |
| 6 | C6 | 523.25 Hz | 72 |
| 7 | C7 | 1046.50 Hz | 84 |
| 8 | C8 | 2093.00 Hz | 96 |

### Duty Cycle & Timbre

The duty cycle determines the waveform shape and resulting timbre:

```
1%  ▃▁▁▁▁▁▁▁▁▁  Narrow pulse - buzzy, nasal, rich harmonics
25% ▃▃▃▁▁▁▁▁▁▁  Asymmetric - clarinet-like
50% ▃▃▃▃▃▁▁▁▁▁  Square wave - hollow, classic chiptune
75% ▃▃▃▃▃▃▃▁▁▁  Asymmetric - mellow
99% ▃▃▃▃▃▃▃▃▃▁  Near-DC - smooth, almost sine-like
```

### Harmonic Intervals

Each PWM channel can be offset by 0-12 semitones from the base note:

| Interval | Semitones | Musical Name |
|----------|-----------|--------------|
| 0 | +0 | Unison |
| 1 | +1 | Minor second |
| 2 | +2 | Major second |
| 3 | +3 | Minor third |
| 4 | +4 | Major third |
| 5 | +5 | Perfect fourth |
| 6 | +6 | Tritone |
| 7 | +7 | Perfect fifth |
| 8 | +8 | Minor sixth |
| 9 | +9 | Major sixth |
| 10 | +10 | Minor seventh |
| 11 | +11 | Major seventh |
| 12 | +12 | Octave |

### Gate Timing

The gate system controls note articulation. Gate duration varies by mode:

| Mode | Gate % | Character |
|------|--------|-----------|
| 0 (Pausa) | 5% | Minimal |
| 1 (Fractal) | 10% | Quick attack |
| 2 (Riu) | 20% | Sustained, fluid |
| 3 (Tempesta) | 8% | Percussive |
| 4 (Harmonia) | 15% | Musical |
| 5 (Bosc) | 18% | Organic |
| 6 (Escala) | 12% | Precise |
| 7 (Euclidia) | 10% | Rhythmic |
| 8 (Cosmos) | 14% | Spacious |

Gate duration is calculated as: `gate_ms = (60000 / BPM) × gate_percentage`

Clamped to 5ms minimum, 200ms maximum.

---

## 6. Parameter Configuration

### Configurable Parameters

Access configuration mode by pressing **Extra 1** (short press). Each press cycles to the next parameter:

| Config Index | Parameter | Range | Description |
|--------------|-----------|-------|-------------|
| 0 | Mode | 0-14 | Current musical mode |
| 1 | Duty 1 | 1-99% | PWM1 duty cycle |
| 2 | Duty 2 | 1-99% | PWM2 duty cycle |
| 3 | Duty 3 | 1-99% | PWM3 duty cycle |
| 4 | Harmonic 1 | 0-12 | PWM1 interval offset |
| 5 | Harmonic 2 | 0-12 | PWM2 interval offset |
| 6 | Harmonic 3 | 0-12 | PWM3 interval offset |

### LED Configuration Indicators

| Config Index | Active LED |
|--------------|------------|
| 0 (Mode) | All LEDs off |
| 1 (Duty1) | LED3 on |
| 2 (Duty2) | LED4 on |
| 3 (Duty3) | LED5 on |
| 4 (Harm1) | LED6 on |
| 5 (Harm2) | LED7 on |
| 6 (Harm3) | LED1 on |

### Adjusting Parameters

1. Press **Extra 1** until desired parameter is selected (watch LEDs)
2. Use **◄ Left** to decrease, **► Right** to increase
3. Hold buttons for acceleration
4. Display shows current value
5. Config mode auto-exits after 6 seconds of inactivity

---

## 7. CV Calibration

### Entering Calibration Mode

1. Hold **Extra 1 + Extra 2** together for 0.5 seconds
2. Display shows "CALIBRACION CV"
3. Live voltage readings appear for CV1 and CV2

### Calibration Presets

| Preset | Range | Use Case |
|--------|-------|----------|
| 0 | 0 - 3.3V | Full range (default) |
| 1 | 0 - 2.5V | Eurorack standard |
| 2 | 0 - 1.5V | Low voltage sources |
| 3 | 0 - 1.0V | Very low voltage |
| 4 | 0 - 0.5V | Minimal range |

### Manual Calibration

In calibration mode, use the D-pad to set min/max values:

- **▲ Up:** Set current CV1 voltage as maximum
- **▼ Down:** Set current CV1 voltage as minimum
- **► Right:** Set current CV2 voltage as maximum
- **◄ Left:** Set current CV2 voltage as minimum

### Why Calibrate?

Calibration ensures the full range of your CV source maps to the full 0-100% parameter range. Without calibration:
- A 0-2.5V Eurorack signal only uses 75% of the range
- LDR sensors with limited swing may only use 20-30%

After calibration, any input voltage maps precisely to the expected parameter range.

---

## 8. Display & Visual Feedback

### Display Modes

**Normal Operation:**
- Large text showing current mode name
- Current octave indicator
- Currently playing note name
- BPM value

**Configuration Mode:**
- Highlighted parameter name
- Current numeric value
- Visual representation (for harmonics: interval name)

**Idle Summary (after 6-9 seconds of inactivity):**
- Large procedural visualization specific to current mode
- Mode 0: Random pixels
- Mode 1: Mandelbrot fractal
- Mode 2: Wave patterns
- Mode 3: Cloud/rain
- Mode 4: Harmonic waveform
- Mode 5: Tree structure
- Mode 6: Scale degrees
- Mode 7: Rhythm grid
- Mode 8: Cosmic rings

**Calibration Mode:**
- "CALIBRACION CV" header
- Live voltage readings for both CV inputs
- Min/Max indicators

### LED System

**LED 2 (Gate Indicator):**
- Illuminates when audio gate is active
- Provides visual click track synchronized with audio
- Duration matches gate length (5-200ms)

**Mode LEDs (Binary Encoding):**
| Mode | LED6 | LED3 | LED7 |
|------|------|------|------|
| 0 | Random pattern (changes every 0.5s) |||
| 1 | off | off | on |
| 2 | off | on | off |
| 3 | off | on | on |
| 4 | on | off | off |
| 5 | on | off | on |
| 6 | on | on | off |
| 7 | on | on | on |
| 8 | off | off | off |
| 9-14 | Mode-specific patterns |

---

## 9. External Clock Input

### Overview

TECLA can synchronize to an external clock source via the CV1 jack. This allows integration with modular synthesizers, drum machines, and other sequencers.

### How It Works

The CV1 input serves dual purposes:
- **Potentiometer (no jack inserted):** Controls internal tempo (20-220 BPM)
- **Jack (external clock):** Receives clock pulses for synchronization

### Clock Detection

The system automatically detects external clock:
1. Rising edge detection with hysteresis (thresholds: LOW < 0.5V, HIGH > 2.0V)
2. Once clock pulses are detected, the system switches to external clock mode
3. Each rising edge triggers one note/step
4. If pulses stop, the system waits (does not fall back to internal tempo)

### Compatible Clock Sources

- Eurorack clock modules (typically 0-5V or 0-10V pulses)
- Drum machine sync outputs
- DAW clock outputs (via audio interface)
- Any pulse signal with voltage swing crossing the 2.0V threshold

### Clock Mode Behavior

| State | Behavior |
|-------|----------|
| No clock detected | CV1 pot controls tempo (internal mode) |
| Clock detected | Notes trigger on each rising edge |
| Clock stops | System waits for next pulse (no fallback) |

**Note:** Once external clock is detected, the system stays in clock mode until power cycle. This ensures stable synchronization without accidental tempo jumps.

---

## 10. MIDI Integration

### USB MIDI Output

TECLA transmits MIDI via USB, allowing integration with DAWs and external synthesizers.

**MIDI Messages Sent:**
- **Note On:** Velocity 100, channel 1
- **Note Off:** Synchronized with gate timing
- **All Notes Off (CC123):** Emergency silence command

### DAW Setup

1. Connect TECLA via USB
2. In your DAW, look for "CircuitPython MIDI" or similar
3. Create a MIDI track with TECLA as input
4. Assign a virtual instrument
5. TECLA's notes will play through your DAW's synth

### Note Mapping

MIDI notes are calculated as:
```
midi_note = (octave × 12) + algorithm_output + harmonic_offset
```

Clamped to 0-127 range.

---

## 11. Quick Reference

### Essential Controls

| Action | Control |
|--------|---------|
| Change mode | ◄ / ► |
| Change octave | ▲ / ▼ |
| Toggle chaos | ▲ at octave 8 / ▼ at octave 0 |
| Control param 1 | Slider |
| Adjust tempo | CV1 pot (when no clock) |
| External clock | CV1 jack (auto-detected) |
| Control param 2 | CV2 / LDR |
| Enter config | Extra 1 (short) |
| Adjust config value | ◄ / ► (in config mode) |
| Show summary | Extra 1 (hold 1.5s) |
| Pause/stop | Extra 2 (hold 1.5s) |
| Calibrate CV | Extra 1 + Extra 2 (hold 0.5s) |

### Mode Summary

| # | Name | Algorithm | CV1 Controls | CV2 Controls |
|---|------|-----------|--------------|--------------|
| 0 | Pausa | Idle | — | — |
| 1 | Fractal | Mandelbrot | X position | Y position |
| 2 | Riu | Sine+cosine | Density | Turbulence |
| 3 | Tempesta | Rain+lightning | Rain intensity | Lightning freq |
| 4 | Harmonia | Chord progression | Root note | Chord type |
| 5 | Bosc | Random jumps | Density | Depth (octaves) |
| 6 | Escala | Modal scale | Mode (0-6) | Step speed |
| 7 | Euclidia | Euclidean rhythm | Pulses | Accents |
| 8 | Cosmos | Hybrid synthesis | Multi-param | Multi-param |
| 9 | Campanetes | Bell arpeggios | Density | Brightness |
| 10 | Segones | Dual voice | Velocity/direction | Interval |
| 11 | Espiral | Scale+transpose | Advance speed | Transpose cycle |
| 12 | Contrapunt | Three voices | Voice 2 density | Voice 3 density |
| 13 | Narval | Call-response | Call probability | Mood |
| 14 | Ciclador | Fixed oscillator | Duty 1 | Duty 2 |

### Default Values

```
Mode:           0 (Pausa)
Octave:         5
BPM:            120
Duty 1/2/3:     50%
Harmonics:      0 (unison)
CV Range:       0-3.3V
```

---

## 12. Troubleshooting

### No Sound

1. **Check mode:** Mode 0 (Pausa) is silent by design
2. **Check BPM:** Slider at minimum = very slow notes
3. **Check gate LED:** LED2 should flash with each note
4. **Check CV range:** Calibrate if CV inputs aren't responding
5. **Check connections:** Verify PWM output wiring

### Notes Hang / Won't Stop

1. **Wait:** Notes should end automatically (gate timing)
2. **Pause:** Hold Extra 2 for 1.5 seconds to enter Mode 0
3. **Reset:** If still stuck, power cycle the device

### Choppy Playback

1. **Lower BPM:** High BPM at complex modes may cause timing issues
2. **Simplify visualization:** Idle animations consume CPU
3. **Disable MIDI:** If not using external gear, MIDI overhead can be reduced

### CV Not Responding

1. **Calibrate:** Enter calibration mode (Extra1+Extra2)
2. **Check range:** Ensure CV preset matches your hardware
3. **Test raw values:** Calibration mode shows live voltage readings
4. **Verify wiring:** Check potentiometer/LDR connections

### Display Issues

1. **Check I2C:** Verify SDA (GP20) and SCL (GP21) connections
2. **Power:** Ensure 3.3V power to display is stable
3. **Address:** Default I2C address is 0x3C for SSD1306

---

## 13. Technical Specifications

### Hardware Platform

| Component | Specification |
|-----------|---------------|
| MCU | RP2040 (Raspberry Pi Pico) |
| CPU | Dual ARM Cortex-M0+ @ 133 MHz |
| RAM | 264 KB SRAM |
| Flash | 2 MB |
| ADC | 12-bit, 3 channels |
| PWM | 16 channels (3 used for audio) |

### Audio Specifications

| Parameter | Value |
|-----------|-------|
| Synthesis | PWM (Pulse Width Modulation) |
| Channels | 3 independent voices |
| Frequency Range | 8 Hz - 4186 Hz |
| Duty Cycle | 1% - 99% |
| Gate Resolution | 5ms - 200ms |
| MIDI Output | USB, Channel 1 |

### Timing

| Parameter | Value |
|-----------|-------|
| BPM Range | 20 - 220 |
| Clock Resolution | ±5ms |
| Gate Precision | ±10ms |
| Button Debounce | 11ms |
| Display Refresh | 150ms |

### Electrical

| Parameter | Value |
|-----------|-------|
| Operating Voltage | 3.3V (from USB 5V) |
| CV Input Range | 0 - 3.3V (configurable) |
| Gate Output | 3.3V TTL |
| PWM Output | 3.3V peak-to-peak |

---

## Appendix A: Musical Theory Reference

### Gregorian Modes (Mode 6)

| Mode | Pattern | Feel |
|------|---------|------|
| Ionian | W-W-H-W-W-W-H | Major, happy |
| Dorian | W-H-W-W-W-H-W | Minor with major 6th |
| Phrygian | H-W-W-W-H-W-W | Dark, Spanish |
| Lydian | W-W-W-H-W-W-H | Bright, floating |
| Mixolydian | W-W-H-W-W-H-W | Bluesy, dominant |
| Aeolian | W-H-W-W-H-W-W | Natural minor |
| Locrian | H-W-W-H-W-W-W | Unstable, diminished |

(W = Whole step, H = Half step)

### Euclidean Rhythms (Mode 7)

Common patterns:

| Pulses/Steps | Pattern | Origin |
|--------------|---------|--------|
| 3/8 | `x..x..x.` | Cuban tresillo |
| 5/8 | `x.xx.xx.` | Cuban cinquillo |
| 7/12 | `x.x.xx.x.xx.` | West African |
| 4/12 | `x..x..x..x..` | Shuffle |
| 5/16 | `x..x.x..x.x..x.x` | Bossa nova |

### Chord Types (Mode 4)

| Type | Intervals | Notes (from C) |
|------|-----------|----------------|
| Major | 0-4-7 | C-E-G |
| Minor | 0-3-7 | C-Eb-G |
| Major 7 | 0-4-7-11 | C-E-G-B |
| Minor 7 | 0-3-7-10 | C-Eb-G-Bb |

---

## Appendix B: Mode Algorithms (Detailed)

### Mandelbrot Algorithm (Mode 1)

```
function mandelbrot_to_midi(cx, cy):
    x, y = 0, 0
    iteration = 0
    max_iter = 200

    while x² + y² ≤ 4 AND iteration < max_iter:
        x_new = x² - y² + cx
        y = 2xy + cy
        x = x_new
        iteration++

    return (iteration mod 60) + 32  // MIDI range 32-92
```

### Euclidean Distribution (Mode 7)

```
function euclidean(pulses, steps):
    pattern = []
    bucket = 0

    for i in 0..steps:
        bucket += pulses
        if bucket >= steps:
            bucket -= steps
            pattern.append(1)  // Hit
        else:
            pattern.append(0)  // Rest

    return pattern
```

---

*TECLA - Transforming mathematics into music since 2024*

*Version 1.0 - Professional Edition*
