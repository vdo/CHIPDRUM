# TECLA BASS

Generative bass & rhythm synth voice for Eurorack, on a Raspberry Pi Pico
(RP2040). C firmware built with the Pico SDK.

Three PWM square-wave oscillators driven by six generative modes: two drum
machines, a Turing-machine bass loop and three drones. External clock input
and master clock output.

## Panel / pin map

| Jack / control | Pin | Function |
|---|---|---|
| **VOICE 1** | GP22 | Bass oscillator (PWM square) |
| **VOICE 2** | GP2 | Sub-octave oscillator |
| **VOICE 3** | GP0 | Accent/fifth oscillator |
| **CLK OUT** | GP1 | Master clock output (5 ms pulses, one per 16th note) |
| **CLK IN** | GP27 (old CV2 jack) | External clock input, read as **analog** with an adaptive threshold |
| **CV1** | GP26 | Macro CV/pot: density, mutation, decay... (per mode) |
| Slider | GP28 | Tempo 30–240 BPM, or clock divide/multiply when external |
| OLED | GP20 SDA / GP21 SCL | SSD1306 128×64, I2C @ 1 MHz |

Buttons: UP GP13 · DOWN GP14 · LEFT GP15 · RIGHT GP3 · EXTRA1 GP5 · EXTRA2 GP4.
LEDs: LED1 GP10 = external clock present · LED2 GP6 = clock pulse ·
LED3–7 = selected parameter slot.

## Clock

- One tick = one 16th note. **CLK OUT emits every tick** — TECLA can be the
  master clock of your rack.
- Patch a clock into **CLK IN** and the engine locks to it (LED1 lights). The
  slider then selects **÷8 ÷6 ÷4 ÷3 ÷2 ×1 ×2 ×3 ×4** — clock multiplication
  works because incoming periods are measured with microsecond precision.
- Unplug the clock and the internal tempo returns after 2 s.

## Modes

| Mode | What it does | CV1 |
|---|---|---|
| **DRUMS** | 32-step sample drum machine: kick, snare and hat one per output jack, driven by Truchets' OG Grids, Electronic and Breakbeat pattern banks | FILL/DENSITY: noon is stable, below drops hits, above adds probabilistic fills |
| **SYNDRUM** | The same banked patterns, but every hit synthesized live — 909-flavoured with an industrial, ring-modulated snare. TUNE and DECAY per drum reach where a fixed sample cannot | FILL/DENSITY, as DRUMS |
| **BASSLOOP** | Turing-machine shift register riff over a root-heavy pitch set {I, I, V↓, I↑}; ends of the knob lock the loop, center mutates | lock ↔ mutate |
| **DRONER** | Three detuned voices on the root, rhythmically chopped, duty-cycle LFO for movement | detune width (cents) |
| **ORGAN** | Harmonic-stack drone: fundamental + pedal + a drawbar harmonic. Three LFOs at irrational rate ratios keep the timbres drifting, so the stack shimmers instead of pulsing | drawbar registration (2nd…8th harmonic) |
| **GRINDER** | Beating/dissonant drone. The third voice sits an INTERVAL away and is offset by CV1 in **hertz**, so the beat rate stays constant as you transpose. PULSE narrows the duty for a reedy grind | beat rate (0.05–8 Hz) |

The three drones share a **CHOP** parameter (HOLD, 8THS, GALLOP, OFFBEAT,
QUARTER, TRIPLET) that rhythmically gates all voices. Their detuning and
beating happen *between* voices, so mix the three outputs to hear it.

The two drum modes share their layout: **V1 = kick, V2 = snare, V3 = hat**,
one per jack. **BANK** selects OG Grids, Electronic or Breakbeat in smooth
interpolated form; **X** and **Y** navigate each bank's 5×5 map.
In DRUMS, the separate **KIT** parameter selects the built-in 909-style kit
or converted sample **BANK 1** / **BANK 2** from `banks/`.

Bass and drone modes carry three global slots — **ROOT** (C–B), **OCT**
(C1–C4) and **GATE** (10–90 % of the step). The drum modes hide them, since
none of the three mean anything to a one-shot.

## Controls

| Button | Short press | Long press |
|---|---|---|
| UP / DOWN | Select slot: MODE → globals (bass modes only) → mode params | — |
| LEFT / RIGHT | Change mode / adjust value (hold to repeat, accelerates) | — |
| EXTRA1 | **Tap**: manual clock tick | Status screen (1.5 s) |
| EXTRA2 | Restart the bar without stopping | Reset mode; in DRUMS/SYNDRUM, hold 2 s to queue random X/Y for the next bar |
| EXTRA1+EXTRA2 | Save root, octave and gate immediately (mode auto-saves after 1 s) | — |

## Building

Requires CMake and the Arm GNU toolchain (`arm-none-eabi-gcc`).

```sh
git clone --depth 1 https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
cd ~/pico-sdk && git submodule update --init --depth 1 lib/tinyusb && cd -
cmake -S . -B build -DPICO_SDK_PATH=$HOME/pico-sdk -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

> The `tinyusb` submodule is required. Without it the SDK **silently** omits
> USB support — the build succeeds but the module has no USB serial and can
> only be re-flashed with the BOOTSEL button.

## Flashing

```sh
./flash.sh
```

Builds and flashes the connected Pico. **No BOOTSEL button needed** — the
script reboots the board into the bootloader for you:

- Already running TECLA BASS → a **1200-baud touch** on its USB serial port
  drops it into the bootloader. This is why USB stdio is enabled: a module
  screwed into a rack can be updated without reaching the PCB.
- Still running CircuitPython → the script reboots it via the REPL
  (`microcontroller.on_next_reset(RunMode.UF2)`).
- Blank board → hold BOOTSEL while plugging in USB, then run the script.

Manual equivalent, once `RPI-RP2` is mounted:

```sh
cp build/tecla_bass.uf2 /Volumes/RPI-RP2/
```

⚠️ Copying the `.uf2` onto a **CIRCUITPY** drive does nothing — that is just
the Python filesystem. A UF2 only flashes via the **RPI-RP2** bootloader
volume. Flashing erases CircuitPython; to go back, flash a CircuitPython
`.uf2` from circuitpython.org the same way.

## Drums: samples and synthesis

**DRUMS** plays 8-bit samples; **SYNDRUM** generates every hit live from
`src/drumsynth.c` — 909-flavoured, with an industrial snare built from
ring-modulated inharmonic partials over a noise tail. Both use the same PWM
DAC path and the same Truchets sequencing, so you can A/B them on identical
patterns. SYNDRUM adds TUNE and DECAY per drum, which reach lengths and
pitches a fixed sample cannot.

The three BANK choices are **OG**, **ELEC**, and **BREAK**. Each bank morphs
smoothly between adjacent nodes as X/Y move. The pattern topology and node
tables are adapted from [Truchets](https://github.com/Dylan-Bolink/eurorack/tree/master/marbles)
and Mutable Instruments Grids under their GPLv3-or-later license.

Everything in the synth runs in fixed point: the Cortex-M0+ has no FPU and
software floats cost around 100 cycles apiece, far too slow inside an
interrupt firing 25,000 times a second.

Preview the synthesized drums on your computer without flashing:

```sh
./tools/preview_synth.sh     # -> samples/preview_{kick,snare,hat}.wav
```

It compiles the real `src/drumsynth.c`, so the WAVs are exactly what the
module produces.

DRUMS plays its samples through the PWM outputs. In sample mode a
voice's PWM slice becomes a DAC: each 8-bit source sample is scaled over 1,250
PWM duty levels with a 100 kHz carrier. A timer interrupt advances all three
voices at 25 kHz; it costs a few percent of one core.

The default **ELEC / X=0 / Y=50** coordinate is Truchets' 4/4 House node. At
FILL/DENSITY noon it gives a stable, playable 32-step groove:

```
kick   x...x...x...x...x...x...x...x...
snare  ....x.......x..x....x.......x..x
hat    x.x.x.x.x.x.x.xxx.x.x.x.x.x.x.xx
```

The knob has a center deadband so 50% never randomizes the groove. Turning it
down removes lower-priority hits deterministically, preserving the strongest
anchors and keeping both bars similar. Turning it up keeps the core intact and
adds lower-priority bank steps with bounded probability, weighted toward the
end of the 32-step phrase; even at 100% it never becomes a deterministic
trigger on every step. Above 60%, hats also gain subtle per-hit velocity
variation, increasing gradually to about 1.2 dB at full FILL. `SWING` runs
from 50% (straight, the default) to 75%
and delays alternate 16ths without moving the master clock. Hold STOP for 2
seconds in either drum mode to randomize X and Y at the next 16-step bar; the
current bar finishes unchanged. A short STOP restarts at step 1 without
freezing playback. `TUNE` shifts kick playback rate in semitones.

The OLED shows a pre-rolled two-bar plan: probabilistic fills are decided
before their steps arrive, so the grid represents what will play rather than
a history of what already played.

To change the kit, put three WAVs (any rate, depth or channel count) through
the converter and rebuild:

```sh
python3 tools/wav2h.py kick.wav:500 snare.wav:300 hat.wav:130 > src/samples.h
```

Regenerate the two additional KIT choices from `banks/` with
`tools/make_sample_banks.sh`. They are converted to unsigned 8-bit mono at
25 kHz, silence-trimmed and faded at the end to prevent clicks.

The `:ms` suffix caps each sample's length — it matters for hats, where a
quiet recording's noise floor survives normalising and leaves a tail that
smears across fast patterns.

The kit is **synthesized rather than sampled**. Every widely circulated
"TR-909 pack" is an unlicensed rip of Roland hardware, so `tools/gen909.py`
models the voices instead: the kick is a fast exponential pitch drop into a
low sine with a click transient, the snare is inharmonic partials plus a noise
tail, the hat is six inharmonic squares through a high-pass. The result is
909-flavoured and carries no licensing baggage. Regenerate with
`tools/make_samples.sh`.

Two things worth knowing: the PWM carrier wants an RC filter (1 kΩ + 10 nF)
per output for a clean reconstruction, and writing settings to flash briefly
suspends playback. Automatic mode saving is deferred until the menu has been
still for one second; use EXTRA1+EXTRA2 between phrases for manual saves.

## Tests

The musical algorithms (Truchets banks and X/Y morphing, Turing register,
clock math, PWM pitch range, clock-input detection and drum
synthesis) can be checked on
your Mac without any hardware — the test compiles the real firmware sources,
so it cannot drift from what the module plays:

```sh
./test/run.sh
```

It asserts that every note the UI can reach fits within the 16-bit PWM wrap;
that the clock input triggers on everything from 3.3 V
logic down to a 0.4 V attenuated gate while ignoring noise; and that every
synth drum is loud, decays to silence and releases its voice at all TUNE and
DECAY extremes — a stuck voice would drone forever on an output jack.

## Architecture

- **Core 0** (realtime): clock engine, clock-input sampling, mode ticks, voice
  glide/sweep updates, buttons, plus the 25 kHz audio interrupt driving the
  sampler and drum synth. Loop runs at hundreds of kHz; ticks land with
  microsecond accuracy.
- **Core 1**: OLED rendering at ~15 FPS (blocking I2C never stalls the audio
  path). Mode `draw()` callbacks run here and only read state.
- PWM voices: fixed clkdiv 128 → 976.6 kHz counters; pitch set via wrap, gate
  via channel level; per-voice glide (time constant) and exponential pitch
  sweeps for percussion.
- Settings persist in the last 4 KB flash sector (`flash_safe_execute`,
  saved only on the button combo to avoid wear).

Adding a mode: create `src/modes/mode_x.c` implementing a `mode_t`
(`reset` / `on_tick` / optional `update` / `draw` + params), add it to
`MODES[]` in `src/modes/modes.c` and to `CMakeLists.txt`.
