# CHIPDRUM

Generative drum, bass & drone voice for Eurorack, on a Raspberry Pi Pico
(RP2040). C firmware built with the Pico SDK.

Three PWM square-wave oscillators driven by six generative modes: two drum
machines, a Turing-machine bass loop and three drones. External clock input
and master clock output.

It runs on stock CHIPTUNE hardware, designed and built by
[TECLA](https://www.instagram.com/tecla.cat/), and replaces the
[CircuitPython firmware](https://github.com/TECLA-code/CHIPTUNE) the module
ships with. Nothing is modified on the PCB — see
[Coming from the original TECLA firmware](#coming-from-the-original-tecla-firmware)
for what changes, and [Reverting](#reverting-to-the-original-tecla-firmware)
for how to put the Python firmware back.

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

- One tick = one 16th note. **CLK OUT emits every tick** — CHIPDRUM can be the
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

- Already running CHIPDRUM → a **1200-baud touch** on its USB serial port
  drops it into the bootloader. This is why USB stdio is enabled: a module
  screwed into a rack can be updated without reaching the PCB.
- Still running the original CircuitPython firmware → the script unmounts
  `CIRCUITPY` and reboots the board through the REPL
  (`microcontroller.on_next_reset(RunMode.UF2)`).
- Blank or unresponsive board → hold BOOTSEL while plugging in USB, then run
  the script.

Manual equivalent, once the `RPI-RP2` bootloader volume is mounted:

```sh
cp build/chipdrum.uf2 /Volumes/RPI-RP2/
```

⚠️ Copying the `.uf2` onto a **CIRCUITPY** drive does nothing — that drive is
the Python filesystem, not the bootloader. A UF2 only flashes through
**RPI-RP2**.

### Coming from the original TECLA firmware

A stock module runs [TECLA-code/CHIPTUNE](https://github.com/TECLA-code/CHIPTUNE):
**Adafruit CircuitPython 10.0.1** plus a Python program — `main.py` and the
`core/`, `modes/`, `display/`, `music/` and `lib/` packages — living on the
`CIRCUITPY` USB drive. The Pico boots the interpreter, the interpreter reads
`main.py` off that filesystem and runs it, so the module is patched by
dragging files onto a drive. That firmware gives you nine algorithmic modes
(Fractal, Riu, Tempesta, Harmonia, Bosc, Escala CV, Euclidia, Cosmos,
Sequenciador) and sends **USB MIDI** alongside the PWM outputs.

This firmware is a native C rewrite on the same hardware, and the difference
is more than the language:

- **No Python is loaded.** `chipdrum.uf2` is bare-metal Cortex-M0+ code
  built against the Pico SDK. There is no interpreter, no import at boot and
  no `main.py`: the module executes the compiled image directly. Changing
  behaviour means editing C and reflashing, not editing a file on a drive.
- **No CIRCUITPY drive.** Flashing overwrites the whole flash — CircuitPython
  and your Python files with it — and nothing is backed up, so copy the drive
  somewhere first if it holds edits you care about. Over USB the module then
  presents a plain serial port (what the 1200-baud reboot uses) and **no USB
  MIDI device**; this firmware talks to the rack through the jacks only.
- **Settings live in the last 4 KB flash sector**, written with
  `flash_safe_execute`, instead of in files on the drive.
- **The panel is re-purposed.** Here CV1 is GP26, the slider GP28, and the old
  CV2 jack (GP27) becomes CLK IN. The CircuitPython firmware read GP28 as BPM,
  GP27 as the mode's second CV and GP26 as the slider.
- **The timing is different in kind:** a 25 kHz fixed-point audio interrupt
  and the clock engine on core 0, the OLED on core 1, where the Python build
  did everything in one interpreted loop.

Nothing is soldered or rewired: same board, same jacks, same OLED. Moving in
either direction is a UF2 copy.

### Reverting to the original TECLA firmware

1. **Get into the bootloader.** With CHIPDRUM running, a 1200-baud touch on
   its serial port does it without reaching the PCB:

   ```sh
   ls /dev/cu.usbmodem*                    # find the module
   stty -f /dev/cu.usbmodemXXXX 1200
   ```

   Otherwise hold **BOOTSEL** while plugging in USB. Either way `RPI-RP2`
   mounts.

2. **Flash CircuitPython.** Download the Raspberry Pi Pico build from
   [circuitpython.org](https://circuitpython.org/board/raspberry_pi_pico/) —
   stock modules shipped **10.0.1** — and copy the `.uf2` across:

   ```sh
   cp ~/Downloads/adafruit-circuitpython-raspberry_pi_pico-en_US-10.0.1.uf2 /Volumes/RPI-RP2/
   ```

   The board reboots and `CIRCUITPY` appears. At this point it is a bare
   CircuitPython board with no TECLA code on it — the OLED stays dark.

3. **Copy the TECLA program back.** It lives in the upstream repository (and
   in this one, on `master`, the last CircuitPython commit before the
   rewrite):

   ```sh
   git clone --depth 1 https://github.com/TECLA-code/CHIPTUNE.git /tmp/tecla-orig
   git -C /tmp/tecla-orig archive HEAD \
       main.py reset.py settings.toml font5x8.bin core modes display music lib fonts \
       | tar -x -C /Volumes/CIRCUITPY
   find /Volumes/CIRCUITPY \( -name '._*' -o -name '.DS_Store' \) -delete
   ```

   About 500 KB — the same files the module shipped with, minus the macOS
   metadata the repository carries. Eject the drive
   (`diskutil eject /Volumes/CIRCUITPY`) and the module restarts on the
   original firmware.

Coming back to this firmware is just `./flash.sh` again: it finds the
CIRCUITPY drive and reboots the board into the bootloader itself.

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

## Audio quality (the drum output)

The bass and drone modes emit a raw square straight from a PWM slice — no
sample rate, no conversion, nothing in the path to measure. The drum modes are
the interesting case, because there the same slice is pressed into service as a
DAC.

During playback the carrier runs at 100 kHz and the 25 kHz audio interrupt
writes each sample into the duty register:

| | |
|---|---|
| Resolution | 1250 duty steps ≈ **10.3 bits** |
| Sample rate | 25 kHz → **12.5 kHz** of bandwidth |
| Measured noise floor | **58.7 dB** below full scale |

That last figure is measured rather than derived: `tools/preview_synth.sh` runs
the synth's output through the same conversion the firmware uses and prints the
error it introduces. It lands at 58.7 dB for kick, snare and hat alike, because
quantization noise does not care which drum it is riding on. Referred to what
you actually hear it is lower — a drum spends most of its length decaying, so
by the end of a kick tail it is closer to 47 dB.

Two things keep it a few dB under the theoretical ceiling. The conversion
truncates instead of rounding, which costs about 6.5 dB and would take one extra
add per sample to fix; and there is no dither, so a long sine-like kick tail
grains rather than hisses.

**Bandwidth.** Hats go soft at the very top. Counting the zero-order hold droop
and the RC filter the outputs want (1 kΩ + 10 nF), the response is flat to
1 kHz, about −1 dB at 5 kHz and −4 dB at 10 kHz. The first image sits ~17 dB
down and the 100 kHz carrier ~16 dB down — harmless into a rack, where
everything downstream is band-limited, but add a second RC pole if you record a
jack straight into an interface.

**DC.** The pin idles at exactly mid-supply, so the drum output carries a
half-supply offset that a low-pass will not remove. That is what a DC-coupled
Eurorack input expects; AC-couple it if you are going somewhere else.

**What limits what.** In SYNDRUM the DAC is the limit. In DRUMS it is not: the
stored samples are unsigned 8-bit, a ~50 dB ceiling, and every 8-bit code still
lands on its own duty step — so there the kit is the limit, not the hardware.
Flash is not what holds it there either, since all three kits together are 64 KB
and the whole firmware occupies 142 KB of the RP2040's 2 MB. A deeper sample
format is affordable if you want one.

In round numbers: **roughly 10-bit audio in a 12.5 kHz band** — a little finer
than the 8-bit drum machines of the early eighties, and close to an SP-1200 in
bandwidth. The grain is a characteristic of the format, not a defect in it.

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

## Acknowledgements

- **Hardware: [TECLA](https://www.instagram.com/tecla.cat/)** — the CHIPTUNE
  module, its PCB, panel and enclosure are TECLA's design and build. This
  firmware exists because that hardware exists; none of it is modified here.
  Find the modules and what else they make at
  [@tecla.cat](https://www.instagram.com/tecla.cat/).
- **Original firmware: [TECLA-code/CHIPTUNE](https://github.com/TECLA-code/CHIPTUNE)**
  — the CircuitPython program the module ships with, and the reference for the
  pin map and hardware behaviour this rewrite had to match. See
  [Coming from the original TECLA firmware](#coming-from-the-original-tecla-firmware)
  for the differences, and [Reverting](#reverting-to-the-original-tecla-firmware)
  to put it back.
- **Pattern banks: [Truchets](https://github.com/Dylan-Bolink/eurorack/tree/master/marbles)
  and Mutable Instruments Grids** — the drum pattern topology and node tables
  are adapted from them, under their GPLv3-or-later license.

CHIPDRUM is an independent, unofficial firmware. It is not affiliated with or
endorsed by TECLA.

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
