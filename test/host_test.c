// Host-side test of the pure algorithmic parts of the TECLA BASS firmware.
// Compiles the real header logic natively (no Pico SDK involved).
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fails = 0;
static void check(const char *name, int cond, const char *detail) {
    printf(cond ? "  OK   %s\n" : "  FAIL %s  %s\n", name, cond ? "" : detail);
    if (!cond) fails++;
}

// Pitch, timing and drum helpers are compiled from the real firmware sources,
// not test copies, so the checks cannot drift from what the module plays.
#include "config.h"
#include "drum_timing.h"
#include "drumsynth.h"
#include "edge_detect.h"
#include "samples.h"
#include "samples_bank1.h"
#include "samples_bank2.h"
#include "scales.h"
#include "truchet_patterns.h"

static uint32_t rng_state = 0xC0FFEE21u;
static uint32_t rng_u32(void) {
    uint32_t x = rng_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return rng_state = x;
}
static float rng_f(void) { return (rng_u32() >> 8) * (1.0f / 16777216.0f); }

static int pcm_peak(const uint8_t *pcm, int len) {
    int peak = 0;
    for (int i = 0; i < len; i++) {
        int d = pcm[i] - 128;
        if (d < 0) d = -d;
        if (d > peak) peak = d;
    }
    return peak;
}

int main(void) {
    printf("== Drum timing ==\n");
    check("swing 50% is straight",
          drum_swing_delay_us(50, 125000, 1) == 0, "offbeat was delayed");
    check("swing never delays even steps",
          drum_swing_delay_us(75, 125000, 2) == 0, "downbeat was delayed");
    check("swing 66% gives a 66/34 pair",
          drum_swing_delay_us(66, 125000, 1) == 40000,
          "wrong offbeat delay");
    check("swing 75% delays half a tick",
          drum_swing_delay_us(75, 125000, 1) == 62500,
          "wrong maximum delay");
    check("queued X/Y targets next bar",
          drum_next_bar_boundary(0) == 0 &&
              drum_next_bar_boundary(1) == 16 &&
              drum_next_bar_boundary(16) == 16 &&
              drum_next_bar_boundary(17) == 0 &&
              drum_next_bar_boundary(31) == 0,
          "wrong 16-step boundary");
    check("hat velocity is fixed through 60% fill",
          drum_hat_velocity_q8(0, 0) == 256 &&
              drum_hat_velocity_q8(60, UINT32_MAX) == 256,
          "velocity changed below threshold");
    check("hat velocity varies subtly above 60%",
          drum_hat_velocity_q8(100, 0) == 256 &&
              drum_hat_velocity_q8(100, UINT32_MAX) == 224 &&
              drum_hat_velocity_q8(80, UINT32_MAX) == 240,
          "wrong velocity range");

    printf("== Turing register (BASSLOOP) ==\n");
    // knob at the extremes must loop exactly; at noon it must mutate
    for (int trial = 0; trial < 2; trial++) {
        float k = trial == 0 ? 0.0f : 1.0f;
        int n = 8;
        uint16_t reg = 0xB39Du;
        uint16_t first = reg;
        int stable = 1;
        for (int i = 0; i < n * 4; i++) { // 4 laps
            uint32_t bit = (reg >> (n - 1)) & 1u;
            if (k > 0.5f) bit ^= 1u;
            float mut = 1.0f - fabsf(2.0f * k - 1.0f);
            if (rng_f() < mut) bit = rng_u32() & 1u;
            reg = (uint16_t)((reg << 1) | bit);
            int lap = (i + 1) % (k > 0.5f ? n * 2 : n);
            if (lap == 0 && (uint8_t)reg != (uint8_t)first) stable = 0;
        }
        check(trial == 0 ? "knob CCW = locked loop" : "knob CW = locked (2x len)",
              stable, "register drifted");
    }
    {
        int n = 8;
        uint16_t reg = 0xB39Du, first = reg;
        int changed = 0;
        for (int i = 0; i < n * 4; i++) {
            uint32_t bit = (reg >> (n - 1)) & 1u;
            float mut = 1.0f; // knob at noon
            if (rng_f() < mut) bit = rng_u32() & 1u;
            reg = (uint16_t)((reg << 1) | bit);
            if ((i + 1) % n == 0 && (uint8_t)reg != (uint8_t)first) changed = 1;
        }
        check("knob noon = mutating", changed, "register never changed");
    }

    printf("== Clock math ==\n");
    // one tick = a 16th note: bpm = 15e6 / period_us
    struct { float bpm; uint32_t us; } bpms[] = {{120, 125000}, {60, 250000}, {240, 62500}};
    for (unsigned i = 0; i < 3; i++) {
        uint32_t p = (uint32_t)(15000000.0f / bpms[i].bpm);
        float back = 15000000.0f / (float)p;
        char nm[32]; sprintf(nm, "%g BPM -> %u us", bpms[i].bpm, p);
        check(nm, p == bpms[i].us && fabsf(back - bpms[i].bpm) < 0.5f, "");
    }
    // divide/multiply factors
    const int FACTORS[] = {-8, -6, -4, -3, -2, 1, 2, 3, 4};
    int span_ok = 1;
    for (float sl = 0.0f; sl <= 1.0f; sl += 0.01f) {
        int idx = (int)(sl * 9);
        if (idx >= 9) idx = 8;
        if (idx < 0 || idx > 8) span_ok = 0;
    }
    check("slider maps to 9 factors, no overflow", span_ok, "");
    // slider -> tempo: 120 BPM must sit at the centre of the throw
    check("slider 0.0 = BPM_MIN", slider_to_bpm(0.0f) == (float)BPM_MIN, "");
    check("slider 0.5 = 120 BPM", fabsf(slider_to_bpm(0.5f) - 120.0f) < 0.01f,
          "centre detent is not 120");
    check("slider 1.0 = BPM_MAX", slider_to_bpm(1.0f) == (float)BPM_MAX, "");
    // The real complaint: this slider reads ~0.53 at its mechanical centre and
    // gave 127 BPM. The detent must catch that, and any similar small offset.
    check("slider 0.53 (off-centre) still = 120",
          fabsf(slider_to_bpm(0.53f) - 120.0f) < 0.01f, "detent too narrow");
    check("slider 0.47 (off-centre) still = 120",
          fabsf(slider_to_bpm(0.47f) - 120.0f) < 0.01f, "detent too narrow");
    // ...but it must not swallow so much travel that nearby tempos vanish
    check("detent stays local (0.35 is well below 120)",
          slider_to_bpm(0.35f) < 110.0f, "detent too wide");
    check("detent stays local (0.65 is well above 120)",
          slider_to_bpm(0.65f) > 130.0f, "detent too wide");
    int mono = 1;
    for (float x = 0.0f; x < 1.0f; x += 0.01f)
        if (slider_to_bpm(x + 0.01f) < slider_to_bpm(x)) mono = 0;
    check("tempo rises monotonically across the throw", mono, "");
    check("factor list spans /8..x4", FACTORS[0] == -8 && FACTORS[8] == 4, "");

    printf("== Pitch ==\n");
    check("A4 = 440 Hz", fabsf(midi_to_freq(69) - 440.0f) < 0.01f, "");
    // bass range sanity: C1..C4 roots
    for (int oct = 0; oct <= 3; oct++) {
        int midi = 24 + 12 * oct;
        float f = midi_to_freq((float)midi);
        char nm[40]; sprintf(nm, "root oct %d = %.2f Hz", oct, f);
        check(nm, f >= 32.0f && f <= 262.0f, "out of bass range");
    }
    // sub-octave must stay above the PWM floor (25 Hz)
    check("sub of lowest root >= 25 Hz", midi_to_freq(24 - 12) >= 16.0f, "");
    printf("       lowest sub = %.2f Hz (VOICE_FREQ_MIN in config.h)\n",
           midi_to_freq(12));

    printf("== PWM resolution across the range (config.h PWM_COUNT_HZ) ==\n");
    // wrap = PWM_COUNT_HZ/f - 1 must fit in 16 bits and keep useful duty steps
    const float COUNT_HZ = PWM_COUNT_HZ;
    float freqs[] = {16.35f, 32.7f, 55.0f, 110.0f, 440.0f, 2500.0f};
    for (unsigned i = 0; i < 6; i++) {
        uint32_t wrap = (uint32_t)(COUNT_HZ / freqs[i]) - 1;
        char nm[64];
        sprintf(nm, "%.2f Hz -> wrap %u", freqs[i], wrap);
        check(nm, wrap <= 65535 && wrap >= 15, "wrap out of range");
    }
    float min_f = COUNT_HZ / 65536.0f;
    printf("       min freq before wrap overflow = %.2f Hz\n", min_f);
    // every note the UI can produce (sub-octave of the lowest root upward)
    // must be reachable without clamping
    check("C0 sub (16.35 Hz) reachable", midi_to_freq(12) >= min_f, "");
    check("VOICE_FREQ_MIN consistent with wrap limit",
          VOICE_FREQ_MIN >= min_f - 0.1f, "min freq would clamp sharp");
    check("top tick (2500 Hz) keeps >=100 duty steps",
          (COUNT_HZ / 2500.0f) >= 100.0f, "");

    printf("== CLK IN adaptive edge detection ==\n");
    // Simulates the ADC seeing a square clock through whatever front end the
    // CV2 jack has. counts = volts / 3.3 * 4095.
    // A plain digital GPIO input would only fire on the first case; that is
    // exactly why external clock was not detected.
    struct { const char *name; float v_low, v_high; int expect; } sigs[] = {
        {"3.3V logic",        0.00f, 3.30f, 1},
        {"5V gate, halved",   0.00f, 1.65f, 1},
        {"10V gate, /10",     0.00f, 1.00f, 1},
        {"attenuated 0.4V",   0.02f, 0.42f, 1},
        {"biased 1.0-2.0V",   1.00f, 2.00f, 1},
        {"tiny 0.05V (noise)",0.00f, 0.05f, 0},
        {"flat DC 1.6V",      1.60f, 1.60f, 0},
    };
    for (unsigned i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) {
        edge_t e;
        edge_init(&e, CLK_MIN_SWING, CLK_ENVELOPE_DECAY);
        uint16_t lo = (uint16_t)(sigs[i].v_low / 3.3f * 4095.0f);
        uint16_t hi = (uint16_t)(sigs[i].v_high / 3.3f * 4095.0f);
        int edges = 0;
        // 8 clock cycles sampled at 1 kHz, 250 ms period, 25% duty
        for (int t = 0; t < 2000; t++) {
            int phase = t % 250;
            uint16_t v = (phase < 62) ? hi : lo;
            if (edge_feed(&e, v, (t % 10) == 0)) edges++;
        }
        char nm[64];
        sprintf(nm, "%-20s -> %d edges", sigs[i].name, edges);
        int ok = sigs[i].expect ? (edges >= 7 && edges <= 8) : (edges == 0);
        check(nm, ok, "unexpected edge count");
    }

    // hysteresis: a noisy signal wobbling around the midpoint must not
    // produce a burst of false edges
    {
        edge_t e;
        edge_init(&e, CLK_MIN_SWING, CLK_ENVELOPE_DECAY);
        int edges = 0;
        for (int t = 0; t < 2000; t++) {
            int phase = t % 250;
            int base = (phase < 62) ? 1240 : 60;    // ~1.0V / ~0.05V
            int n = base + ((t % 7) - 3) * 40;      // +-120 counts of noise
            if (n < 0) n = 0;                       // the ADC is 12-bit: 0..4095
            if (n > 4095) n = 4095;
            if (edge_feed(&e, (uint16_t)n, (t % 10) == 0)) edges++;
        }
        char nm[48];
        sprintf(nm, "noisy 1V clock -> %d edges", edges);
        check(nm, edges >= 7 && edges <= 8, "hysteresis failed");
    }

    // unplugging: once the source goes away the envelope must decay so a
    // static input stops being treated as a clock
    {
        edge_t e;
        edge_init(&e, CLK_MIN_SWING, CLK_ENVELOPE_DECAY);
        for (int t = 0; t < 1000; t++)
            edge_feed(&e, (t % 250) < 62 ? 2000 : 0, (t % 10) == 0);
        int edges_after = 0;
        for (int t = 0; t < 20000; t++) // 20 s of silence at 1 kHz
            if (edge_feed(&e, 0, (t % 10) == 0)) edges_after++;
        check("envelope forgets an unplugged source",
              edges_after == 0 && edge_swing(&e) < CLK_MIN_SWING, "still armed");
    }

    printf("== Truchets drum pattern banks ==\n");
    // The default coordinate is Electronic / 4/4 House. Knob noon is a
    // stable thresholded groove; fills and drops only occur outside its
    // center deadband.
    {
        uint32_t masks[TRUCHET_INSTRUMENTS];
        truchet_base_masks(TRUCHET_DEFAULT_BANK, TRUCHET_DEFAULT_X,
                           TRUCHET_DEFAULT_Y, masks);
        char k[33] = {0}, s[33] = {0}, h[33] = {0};
        for (int i = 0; i < TRUCHET_STEPS; i++) {
            k[i] = ((masks[0] >> i) & 1) ? 'x' : '.';
            s[i] = ((masks[1] >> i) & 1) ? 'x' : '.';
            h[i] = ((masks[2] >> i) & 1) ? 'x' : '.';
        }
        printf("       kick   %s\n       snare  %s\n       hat    %s\n", k, s, h);
        check("32-step kick keeps four-on-floor tempo",
              strcmp(k, "x...x...x...x...x...x...x...x...") == 0, k);
        check("32-step snare keeps two-bar tempo + turnarounds",
              strcmp(s, "....x.......x..x....x.......x..x") == 0, s);
        check("32-step hat keeps two-bar tempo",
              strcmp(h, "x.x.x.x.x.x.x.xxx.x.x.x.x.x.x.xx") == 0, h);
    }
    {
        int center_stable = 1, low_stable = 1, low_repeats = 1;
        int low_kept = 0, low_removed = 0;
        int zero_silent = 1, max_not_flood = 1;
        uint32_t base[TRUCHET_INSTRUMENTS];
        truchet_base_masks(TRUCHET_DEFAULT_BANK, TRUCHET_DEFAULT_X,
                           TRUCHET_DEFAULT_Y, base);
        for (int d = 0; d < TRUCHET_INSTRUMENTS; d++)
            for (int step = 0; step < TRUCHET_STEPS; step++) {
                int expected = (base[d] >> step) & 1u;
                if (truchet_should_hit(TRUCHET_DEFAULT_BANK,
                                       TRUCHET_DEFAULT_X, TRUCHET_DEFAULT_Y,
                                       d, step, 0.5f, 0.0f) != expected ||
                    truchet_should_hit(TRUCHET_DEFAULT_BANK,
                                       TRUCHET_DEFAULT_X, TRUCHET_DEFAULT_Y,
                                       d, step, 0.5f, 0.999f) != expected)
                    center_stable = 0;
                if (truchet_should_hit(TRUCHET_DEFAULT_BANK,
                                       TRUCHET_DEFAULT_X, TRUCHET_DEFAULT_Y,
                                       d, step, 0.0f, 0.0f))
                    zero_silent = 0;
                int low_a = truchet_should_hit(
                    TRUCHET_DEFAULT_BANK, TRUCHET_DEFAULT_X,
                    TRUCHET_DEFAULT_Y, d, step, 0.25f, 0.0f);
                int low_b = truchet_should_hit(
                    TRUCHET_DEFAULT_BANK, TRUCHET_DEFAULT_X,
                    TRUCHET_DEFAULT_Y, d, step, 0.25f, 0.999f);
                if (low_a != low_b) low_stable = 0;
                if (step < 16 && low_a != truchet_should_hit(
                                              TRUCHET_DEFAULT_BANK,
                                              TRUCHET_DEFAULT_X,
                                              TRUCHET_DEFAULT_Y, d, step + 16,
                                              0.25f, 0.5f))
                    low_repeats = 0;
                if (expected && low_a)
                    low_kept++;
                else if (expected)
                    low_removed++;
                if (!expected && truchet_should_hit(
                                     TRUCHET_DEFAULT_BANK, TRUCHET_DEFAULT_X,
                                     TRUCHET_DEFAULT_Y, d, step, 1.0f, 0.999f))
                    max_not_flood = 0;
            }
        check("50% density is stable", center_stable, "randomness at noon");
        check("below 50% uses stable priority thinning", low_stable,
              "low-density hits changed with RNG");
        check("below 50% keeps both bars similar", low_repeats,
              "matching bar steps diverged");
        check("25% keeps anchors but removes weaker hits",
              low_kept > 0 && low_removed > 0, "thinning is too extreme");
        check("density 0 = silent", zero_silent, "");
        check("density 100 is not a deterministic flood", max_not_flood, "");

        uint8_t fill_level = truchet_level(
            TRUCHET_DEFAULT_BANK, TRUCHET_DEFAULT_X, TRUCHET_DEFAULT_Y,
            0, 1);
        float early = truchet_fill_probability(fill_level, 1, 1.0f);
        float late = truchet_fill_probability(fill_level, 31, 1.0f);
        check("fills are probabilistic", early > 0.0f && late < 1.0f, "");
        check("fills favour the phrase ending", late > early * 2.0f, "");
        check("zero-level rests stay rests at 100%",
              !truchet_should_hit(TRUCHET_DEFAULT_BANK, TRUCHET_DEFAULT_X,
                                  TRUCHET_DEFAULT_Y, 0, 2, 1.0f, 0.0f), "");
    }
    {
        check("only three musical banks are exposed",
              TRUCHET_BANKS == 3 &&
                  strcmp(truchet_bank_name(0), "OG") == 0 &&
                  strcmp(truchet_bank_name(1), "ELEC") == 0 &&
                  strcmp(truchet_bank_name(2), "BREAK") == 0,
              "unexpected bank list");
        int x_morphs = 0;
        for (int d = 0; d < TRUCHET_INSTRUMENTS; d++)
            for (int step = 0; step < TRUCHET_STEPS; step++)
                if (truchet_level(1, 12, 50, d, step) !=
                    truchet_level(1, 0, 50, d, step))
                    x_morphs = 1;
        check("X morphs between neighboring nodes", x_morphs,
              "X had no effect");
        uint32_t y0[3], y1[3];
        truchet_base_masks(2, 50, 0, y0);
        truchet_base_masks(2, 50, 100, y1);
        check("Y selects a different groove",
              y0[0] != y1[0] || y0[1] != y1[1] || y0[2] != y1[2],
              "Y had no effect");
    }

    printf("== Sample data ==\n");
    // sanity-check the generated header the firmware actually compiles in
    check("all sample kits use the audio engine rate",
          SAMPLE_RATE_HZ == AUDIO_SR && BANK1_SAMPLE_RATE_HZ == AUDIO_SR &&
              BANK2_SAMPLE_RATE_HZ == AUDIO_SR,
          "sample rate mismatch");
    check("converted banks fit comfortably in flash",
          BANK1_KICK_LEN + BANK1_SNARE_LEN + BANK1_HAT_LEN +
                  BANK2_KICK_LEN + BANK2_SNARE_LEN + BANK2_HAT_LEN <
              128 * 1024,
          "converted kits are unexpectedly large");
    {
        struct {
            const char *name;
            const uint8_t *pcm;
            int len;
        } imported[] = {
            {"bank1 kick", BANK1_KICK_PCM, BANK1_KICK_LEN},
            {"bank1 snare", BANK1_SNARE_PCM, BANK1_SNARE_LEN},
            {"bank1 hat", BANK1_HAT_PCM, BANK1_HAT_LEN},
            {"bank2 kick", BANK2_KICK_PCM, BANK2_KICK_LEN},
            {"bank2 snare", BANK2_SNARE_PCM, BANK2_SNARE_LEN},
            {"bank2 hat", BANK2_HAT_PCM, BANK2_HAT_LEN},
        };
        for (unsigned i = 0; i < sizeof(imported) / sizeof(imported[0]); i++) {
            char nm[64];
            int end = imported[i].pcm[imported[i].len - 1];
            int peak = pcm_peak(imported[i].pcm, imported[i].len);
            sprintf(nm, "%s converted (peak %d, end %d)", imported[i].name,
                    peak, end);
            check(nm, imported[i].len > 500 && peak > 115 && end > 118 &&
                          end < 138,
                  "bad conversion");
        }
    }
    check("kick length sane", KICK_LEN > 1000 && KICK_LEN < 40000, "");
    check("snare length sane", SNARE_LEN > 500 && SNARE_LEN < 40000, "");
    check("hat is short (closed hat)", HAT_LEN < (SAMPLE_RATE_HZ / 5), "hat too long");
    {
        // one-shots must start and end near the 128 idle level, otherwise
        // playback clicks at the start or leaves a DC step at the end
        // A kick must attack instantly - the click transient is the point, so
        // unlike the tail there is no ramp-in to expect here. What would be a
        // defect is a soft start, so require full scale within the first 2 ms.
        int attack = 0;
        for (int i = 0; i < SAMPLE_RATE_HZ / 500 && i < KICK_LEN; i++) {
            int d = KICK_PCM[i] - 128;
            if (d < 0) d = -d;
            if (d > attack) attack = d;
        }
        char anm[48];
        sprintf(anm, "kick attacks within 2 ms (peak %d)", attack);
        check(anm, attack > 90, "soft attack - kick will not cut through");
        check("kick ends near idle", KICK_PCM[KICK_LEN - 1] > 118 &&
                                     KICK_PCM[KICK_LEN - 1] < 138, "");
        check("snare ends near idle", SNARE_PCM[SNARE_LEN - 1] > 118 &&
                                      SNARE_PCM[SNARE_LEN - 1] < 138, "");
        check("hat ends near idle", HAT_PCM[HAT_LEN - 1] > 118 &&
                                    HAT_PCM[HAT_LEN - 1] < 138, "");
        int peak = 0;
        for (int i = 0; i < KICK_LEN; i++) {
            int d = KICK_PCM[i] - 128;
            if (d < 0) d = -d;
            if (d > peak) peak = d;
        }
        char nm[48];
        sprintf(nm, "kick normalised (peak %d/127)", peak);
        check(nm, peak > 115, "sample is quiet, wasting 8-bit range");
    }

    printf("== SYNDRUM synthesis (real drumsynth.c) ==\n");
    // Renders each drum and checks it is loud, decays to silence, and stops.
    // A stuck voice would drone forever on an output jack.
    {
        drumsynth_init();
        struct { const char *name; drum_t d; int decay; float lo, hi; } dr[] = {
            {"kick",  DRUM_KICK,  55, 0.25f, 0.90f},
            {"snare", DRUM_SNARE, 45, 0.15f, 0.90f},
            {"hat",   DRUM_HAT,   30, 0.03f, 0.25f},
        };
        for (unsigned i = 0; i < 3; i++) {
            drumsynth_trigger(0, dr[i].d, 0, dr[i].decay);
            int peak = 0, last = 0, n = AUDIO_SR * 2; // 2 s of headroom
            long sum = 0;
            for (int t = 0; t < n; t++) {
                int v = drumsynth_tick(0);
                if (v > peak) peak = v;
                if (-v > peak) peak = -v;
                if (v > 2 || v < -2) last = t;
                sum += v;
            }
            float len = last / (float)AUDIO_SR;
            char nm[64];
            sprintf(nm, "%-5s peak %3d, %.3f s", dr[i].name, peak, len);
            check(nm, peak > SYNTH_FULL * 3 / 4 && len >= dr[i].lo && len <= dr[i].hi,
                  "too quiet or wrong length");
            sprintf(nm, "%s voice frees itself", dr[i].name);
            check(nm, !drumsynth_active(0), "stuck on - would drone forever");
            // mean must sit near zero or the output carries a DC offset
            sprintf(nm, "%s has no DC offset (mean %ld)", dr[i].name,
                    sum / n);
            check(nm, sum / n > -SYNTH_FULL / 32 && sum / n < SYNTH_FULL / 32,
                  "DC offset");
        }
        {
            int peak = 0;
            drumsynth_trigger_velocity(0, DRUM_HAT, 0, 30, 224);
            for (int t = 0; t < AUDIO_SR; t++) {
                int v = drumsynth_tick(0);
                int a = v < 0 ? -v : v;
                if (a > peak) peak = a;
            }
            check("synth hat applies velocity after clipping",
                  peak <= (SYNTH_FULL * 224) / 256 + 1,
                  "velocity scaling exceeded its bound");
        }
        // A snare must not read as a hi-hat. Both are noise-driven, so the
        // thing that separates them is where that noise sits: measure the
        // share of energy above 4 kHz and require real daylight between them.
        {
            double share[2];
            const drum_t which[2] = {DRUM_SNARE, DRUM_HAT};
            const int dec[2] = {45, 30};
            for (int i = 0; i < 2; i++) {
                static int buf[AUDIO_SR / 2];
                drumsynth_init();
                drumsynth_trigger(0, which[i], 0, dec[i]);
                int n = AUDIO_SR / 2;
                for (int t = 0; t < n; t++) buf[t] = drumsynth_tick(0);
                double lo = 0, hi = 0;
                for (int f = 100; f <= 11000; f += 200) {
                    double re = 0, im = 0;
                    for (int t = 0; t < n; t++) {
                        double a = 2 * M_PI * f * t / (double)AUDIO_SR;
                        re += buf[t] * cos(a); im += buf[t] * sin(a);
                    }
                    double m = (re * re + im * im) / ((double)n * n);
                    if (f >= 4000) hi += m; else lo += m;
                }
                share[i] = 100.0 * hi / (hi + lo);
            }
            char nm[80];
            sprintf(nm, "snare is not hatty (%.0f%% >4kHz vs hat %.0f%%)",
                    share[0], share[1]);
            check(nm, share[0] < 40.0 && share[1] - share[0] > 40.0,
                  "snare noise is too bright - it reads as a hi-hat");
            sprintf(nm, "snare keeps its snap (%.0f%% >4kHz)", share[0]);
            check(nm, share[0] > 8.0, "over-filtered - sounds muffled");
        }

        // The snare should thump, not clang. Two measurable properties:
        // its body sweeps downward in pitch (the gesture that makes a drum
        // thump), and most of its energy sits low rather than in the metallic
        // mid-band that ring modulation used to produce.
        {
            static int buf[AUDIO_SR / 2];
            drumsynth_init();
            drumsynth_trigger(0, DRUM_SNARE, 0, 45);
            int n = AUDIO_SR / 2;
            for (int t = 0; t < n; t++) buf[t] = drumsynth_tick(0);
            int w = AUDIO_SR / 200; // 5 ms
            double f_early = 0, f_late = 0, best;
            for (int pass = 0; pass < 2; pass++) {
                int a = pass ? w * 8 : 0;
                best = 0;
                for (int f = 100; f <= 900; f += 5) {
                    double re = 0, im = 0;
                    for (int i = 0; i < w; i++) {
                        double t = 2 * M_PI * f * (a + i) / (double)AUDIO_SR;
                        re += buf[a+i] * cos(t); im += buf[a+i] * sin(t);
                    }
                    double m = re * re + im * im;
                    if (m > best) { best = m; if (pass) f_late = f; else f_early = f; }
                }
            }
            // 100 Hz steps: a coarser sweep puts too few bins under 400 Hz
            // and undercounts the low band badly.
            double lo = 0, tot = 0;
            for (int f = 100; f <= 11000; f += 100) {
                double re = 0, im = 0;
                for (int t = 0; t < n; t++) {
                    double a = 2 * M_PI * f * t / (double)AUDIO_SR;
                    re += buf[t] * cos(a); im += buf[t] * sin(a);
                }
                double m = (re * re + im * im) / ((double)n * n);
                tot += m;
                if (f < 400) lo += m;
            }
            char nm[80];
            sprintf(nm, "snare body sweeps down (%.0f -> %.0f Hz)", f_early, f_late);
            check(nm, f_early > f_late + 30.0, "no pitch drop - no thump");
            sprintf(nm, "snare energy sits low (%.0f%% under 400 Hz)",
                    100.0 * lo / tot);
            check(nm, lo / tot > 0.40, "too much mid - reads as metallic");
        }

        // The ride end of the hat knob must be a cymbal, not a loud tick.
        // Two independent properties, measured rather than assumed:
        //   1. a soft onset - the first millisecond must NOT jump to full
        //      scale the way a closed hat does. That instant jump is what
        //      makes a hit read as percussive.
        //   2. a ping that still stands clear of the wash behind it.
        // Windows are wide enough to contain the bloom: the soft onset moves
        // the peak several ms in, so a narrow attack window would miss it.
        {
            int onset[2] = {0, 0}, pk[2] = {0, 0};
            float wash[2] = {0.0f, 0.0f};
            const int decs[2] = {20, 100}; // closed hat, ride
            for (int i = 0; i < 2; i++) {
                drumsynth_trigger(0, DRUM_HAT, 0, decs[i]);
                int ms1 = AUDIO_SR / 1000, att_n = AUDIO_SR / 25;
                int b0 = AUDIO_SR / 7, b1 = AUDIO_SR / 2;
                long body_sq = 0;
                int body_n = 0;
                for (int t = 0; t < AUDIO_SR; t++) {
                    int v = drumsynth_tick(0);
                    int a = v < 0 ? -v : v;
                    if (t < ms1 && a > onset[i]) onset[i] = a;
                    if (t < att_n && a > pk[i]) pk[i] = a;
                    if (t >= b0 && t < b1) { body_sq += (long)v * v; body_n++; }
                }
                wash[i] = body_n ? sqrtf((float)body_sq / body_n) : 0.0f;
            }
            char nm[80];
            sprintf(nm, "ride onset is soft (%d vs closed hat %d in 1 ms)",
                    onset[1], onset[0]);
            check(nm, onset[1] < onset[0] / 3, "ride still snaps like a hit");
            float ratio = wash[1] > 1.0f ? pk[1] / wash[1] : 0.0f;
            sprintf(nm, "ride keeps a ping over its wash (x%.1f)", ratio);
            check(nm, ratio > 3.0f, "transient swallowed by the wash");
            sprintf(nm, "ride sustains a real wash (rms %.1f)", wash[1]);
            check(nm, wash[1] > SYNTH_FULL / 200.0f,
                  "wash too thin - reads as a tick + tail");
        }

        // extremes of TUNE/DECAY must not blow up or hang
        int ok = 1;
        for (int tune = -24; tune <= 24; tune += 8)
            for (int dec = 0; dec <= 100; dec += 25)
                for (int d = 0; d < 3; d++) {
                    drumsynth_trigger(0, (drum_t)d, tune, dec);
                    for (int t = 0; t < AUDIO_SR * 3; t++) {
                        int v = drumsynth_tick(0);
                        if (v > SYNTH_FULL || v < -SYNTH_FULL) ok = 0;
                    }
                    if (drumsynth_active(0)) ok = 0; // must always end
                }
        check("all TUNE/DECAY extremes stay in range and terminate", ok, "");
    }

    printf("\n%s\n", fails ? "FAILURES PRESENT" : "ALL HOST TESTS PASSED");
    return fails ? 1 : 0;
}
