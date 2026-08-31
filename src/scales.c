#include "scales.h"

#include <math.h>
#include <stdio.h>

#include "config.h"

float midi_to_freq(float midi) {
    return 440.0f * powf(2.0f, (midi - 69.0f) / 12.0f);
}

static const char *const NAMES[12] = {"C",  "C#", "D",  "D#", "E",  "F",
                                      "F#", "G",  "G#", "A",  "A#", "B"};

void note_name(int midi, char *buf) {
    if (midi < 0 || midi > 127) {
        buf[0] = '?';
        buf[1] = 0;
        return;
    }
    sprintf(buf, "%s%d", NAMES[midi % 12], midi / 12 - 1);
}

float slider_to_bpm(float norm) {
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    // Two linear segments running to the edges of a centre detent, so tempo
    // stays continuous across the flat spot instead of jumping out of it.
    const float lo = 0.5f - SLIDER_DETENT;
    const float hi = 0.5f + SLIDER_DETENT;
    if (norm <= lo) return BPM_MIN + (BPM_CENTER - BPM_MIN) * (norm / lo);
    if (norm >= hi)
        return BPM_CENTER + (BPM_MAX - BPM_CENTER) * ((norm - hi) / (1.0f - hi));
    return BPM_CENTER;
}
