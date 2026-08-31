// Render the module's synth drums to WAV files so they can be auditioned on a
// computer. Compiles the real src/drumsynth.c, so what you hear is exactly
// what the module generates.
//
//   sh tools/preview_synth.sh   ->  samples/preview_{kick,snare,hat}.wav
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "drumsynth.h"

// Take the rate from the firmware, never a copy: a stale value here would
// render previews at the wrong pitch while still claiming to match the module.
#include "config.h"
#define SR AUDIO_SR

static void write_wav(const char *path, const int16_t *s, int n) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return; }
    int data = n * 2, riff = 36 + data;
    fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
    int fmt = 16; short pcm = 1, ch = 1, bits = 16, align = 2;
    int rate = SR, bps = SR * 2;
    fwrite(&fmt, 4, 1, f); fwrite(&pcm, 2, 1, f); fwrite(&ch, 2, 1, f);
    fwrite(&rate, 4, 1, f); fwrite(&bps, 4, 1, f);
    fwrite(&align, 2, 1, f); fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&data, 4, 1, f);
    for (int i = 0; i < n; i++) {
        short v = (short)(s[i] * 16);
        fwrite(&v, 2, 1, f);
    }
    fclose(f);
}

int main(void) {
    drumsynth_init();
    static const char *names[4] = {"kick", "snare", "hat", "ride"};
    static const drum_t drums[4] = {DRUM_KICK, DRUM_SNARE, DRUM_HAT, DRUM_HAT};
    // the fourth is the hat at full DECAY: it should open into a ride wash
    static const int decay[4] = {55, 45, 30, 100};

    for (int d = 0; d < 4; d++) {
        static int16_t buf[SR * 3]; // up to three seconds
        memset(buf, 0, sizeof(buf));
        drumsynth_trigger(0, drums[d], 0, decay[d]);
        int last = 0, peak = 0;
        int clip = 0;
        long att_sq = 0, tail_sq = 0, quant_err = 0; int att_pk = 0, ms1_pk = 0, tail_n = 0;
        // The ping no longer sits at sample zero: the soft onset moves it a
        // few ms in, so the attack window has to be wide enough to contain it
        // and the body window has to start after it has passed.
        int att_n = SR / 25;            // first 40 ms
        int body_a = SR / 7, body_b = SR / 2; // 140..500 ms
        for (int i = 0; i < SR * 3; i++) {
            int ideal = drumsynth_tick(0);
            // Emulate the DAC so the preview is what the module OUTPUTS, not
            // what the synth computes: quantise to the PWM's duty steps and
            // convert back. Without this the preview hides exactly the
            // quantization artifacts we are trying to hear.
            int level = DAC_FROM_SYNTH(ideal);
            int v = ((level * 4096) / DAC_STEPS) - 2048;
            quant_err += (long)(v - ideal) * (v - ideal);
            buf[i] = (int16_t)v;
            if (v > peak) peak = v;
            if (v < -peak) peak = -v;
            if (v > 2 || v < -2) last = i;
            if (v >= SYNTH_FULL - 1 || v <= -(SYNTH_FULL - 1)) clip++;
            if (i < SR / 1000) { if (v > ms1_pk) ms1_pk = v; if (-v > ms1_pk) ms1_pk = -v; }
            if (i < att_n) { att_sq += (long)v * v; if (v > att_pk) att_pk = v; if (-v > att_pk) att_pk = -v; }
            else if (i >= body_a && i < body_b) { tail_sq += (long)v * v; tail_n++; }
        }
        // attack-to-body ratio: how much the stick transient stands proud of
        // the wash that follows it
        double att = att_sq ? sqrt(att_sq / (double)att_n) : 0;
        double tail = tail_n ? sqrt(tail_sq / (double)tail_n) : 0;
        char path[64];
        sprintf(path, "samples/preview_%s.wav", names[d]);
        write_wav(path, buf, last + 1);
        printf("%-6s peak %5d/2047   len %.3f s   attackRMS %.1f body %.1f"
               "  ping/wash x%.2f  onset %4d  clip %d  quantSNR %.0fdB\n", names[d],
               peak, (last + 1) / (double)SR, att, tail,
               tail > 0 ? att_pk / tail : 0, ms1_pk, clip,
               quant_err ? 20 * log10(2047.0 / sqrt(quant_err / (double)(last + 1))) : 99.0);
    }
    return 0;
}
