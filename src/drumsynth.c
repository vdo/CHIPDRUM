#include "drumsynth.h"

#include <math.h>
#include <stdbool.h>

// __not_in_flash_func keeps the ISR body in RAM on the Pico. When this file
// is compiled on the host (tests, preview renderer) it is just a no-op, so the
// same code that runs on the module is what gets verified.
#if defined(PICO_NO_HARDWARE) || !defined(__arm__)
#define __not_in_flash_func(f) f
#else
#include "pico.h"
#endif

#include "config.h"

// --- Fixed point conventions ---
// Phase accumulators are 32-bit; the top 8 bits index the sine table, so a
// phase increment of (f * 2^32 / SR) sweeps one cycle per period.
// Envelopes are Q16: level starts at 1<<16 and is multiplied by a per-sample
// coefficient just under 1<<16, giving exponential decay with one multiply.

#define ENV_ONE 65536
#define SR AUDIO_SR

static int16_t sine_tab[256];
static uint32_t rng = 0x2545F491u;

// 12-bit noise, matching the synth's internal range
static inline int32_t noise(void) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (int32_t)((rng >> 12) & 0xFFF) - 2048; // -2048..2047
}

// Per-sample decay coefficient for a given time constant, as Q16.
// coef = exp(-1 / (tau * SR)) scaled by 65536.
static uint32_t tau_to_coef(float tau_s) {
    if (tau_s < 0.002f) tau_s = 0.002f;
    float c = expf(-1.0f / (tau_s * (float)SR));
    uint32_t q = (uint32_t)(c * 65536.0f);
    if (q > 65535) q = 65535; // must decay, never hold
    return q;
}

static inline uint32_t hz_to_inc(float hz) {
    return (uint32_t)(hz * 4294967296.0f / (float)SR);
}

typedef struct {
    drum_t drum;
    bool active;

    uint32_t ph, ph2, ph3;
    uint32_t inc, inc2, inc3;

    // amplitude envelope
    uint32_t env, env_coef;
    // pitch envelope (kick): sweeps inc from inc_start down to inc_end
    uint32_t pinc, pinc_end, penv, penv_coef, pinc_span;
    // noise envelope (snare)
    uint32_t nenv, nenv_coef;

    uint32_t click; // kick transient countdown
    int32_t hp_prev_x, hp_prev_y;
    int32_t hp2_prev_x, hp2_prev_y; // second pole, for a steeper hat filter
    int32_t hp_a;                   // hat filter coefficient, Q8
    uint32_t aenv, aenv_coef;       // hat stick attack
    int32_t attack_gain, body_gain;
    uint16_t velocity;                   // Q8 amplitude, 256 = unity
    uint32_t ramp, ramp_step;       // soft onset, for the ride end of the knob
    int32_t lp_y, lp2_y;            // low-pass state: kick tone, snare wires
} dvoice_t;

static dvoice_t dv[NUM_VOICES];

void drumsynth_init(void) {
    for (int i = 0; i < 256; i++)
        sine_tab[i] = (int16_t)(sinf(i * 6.2831853f / 256.0f) * SYNTH_FULL);
    for (int v = 0; v < NUM_VOICES; v++) dv[v].active = false;
}

static inline float semis(int tune) { return powf(2.0f, tune / 12.0f); }

void drumsynth_trigger_velocity(int voice, drum_t drum, int tune, int decay,
                                uint16_t velocity_q8) {
    dvoice_t *d = &dv[voice];
    float k = semis(tune);
    float dec = decay / 100.0f; // 0..1

    d->drum = drum;
    d->velocity = velocity_q8 > 256 ? 256 : velocity_q8;
    d->env = ENV_ONE;
    d->ph = d->ph2 = d->ph3 = 0;
    d->hp_prev_x = d->hp_prev_y = 0;

    switch (drum) {
    case DRUM_KICK: {
        // fast exponential pitch drop, the defining 909 gesture
        float f0 = 210.0f * k, f1 = 47.0f * k;
        d->pinc = hz_to_inc(f0);
        d->pinc_end = hz_to_inc(f1);
        d->pinc_span = d->pinc - d->pinc_end;
        d->penv = ENV_ONE;
        d->penv_coef = tau_to_coef(0.024f);
        d->env_coef = tau_to_coef(0.05f + dec * 0.25f); // 50..300 ms
        d->click = 90;                                  // ~4 ms of transient
        d->lp_y = 0;
        break;
    }
    case DRUM_SNARE: {
        // A thump, not a clang. The metallic character came from ring
        // modulating two inharmonic partials; that is gone. What makes a drum
        // thump is a fast downward pitch sweep in the body - the same gesture
        // as the kick, just higher and quicker - with the wires sitting on
        // top rather than defining the sound.
        d->pinc = hz_to_inc(330.0f * k);
        d->pinc_end = hz_to_inc(172.0f * k);
        d->pinc_span = d->pinc - d->pinc_end;
        d->penv = ENV_ONE;
        d->penv_coef = tau_to_coef(0.013f);
        d->env_coef = tau_to_coef(0.03f + dec * 0.10f);   // body: 30..130 ms
        d->nenv = ENV_ONE;
        d->nenv_coef = tau_to_coef(0.02f + dec * 0.13f);  // wires: 20..150 ms
        d->lp_y = d->lp2_y = 0;
        break;
    }
    default: {
        // The 909 hat is noise, not the 808's ringing oscillator bank, so
        // there is nothing tonal here to detune - TUNE shifts the filter
        // corner instead, which is what changes a noise hat's character.
        //
        // DECAY is squared so the low half stays in tight closed-hat
        // territory and only the top opens out into a ride-length wash. The
        // filter corner drops as it lengthens: a closed hat is all top end,
        // while a ride needs body to sound like metal rather than hiss.
        float len = dec * dec;
        d->env_coef = tau_to_coef(0.005f + len * 0.32f); // 5 ms .. ~2.5 s
        float fc = (7600.0f - len * 4900.0f) * k;
        if (fc < 600.0f) fc = 600.0f;
        if (fc > 10000.0f) fc = 10000.0f;
        d->hp_a = (int32_t)(256.0f / (1.0f + 6.2831853f * fc / (float)SR));
        if (d->hp_a < 1) d->hp_a = 1;
        d->hp2_prev_x = d->hp2_prev_y = 0;

        // Stick attack. The wash alone reads as hiss; a cymbal needs the
        // transient of the stick landing on it. It uses unfiltered noise, so
        // it carries the body the high-passed tail deliberately throws away.
        // Scaled by DECAY: a closed hat is already nothing but transient,
        // while a long ride needs a distinct ping in front of the wash.
        d->aenv = ENV_ONE;
        // The ping blooms rather than snaps as the knob opens: a closed hat
        // is a 6 ms tick, a ride swells over ~40 ms. Same energy, far less
        // percussive.
        d->aenv_coef = tau_to_coef(0.006f + dec * 0.035f);
        // Balance tracks DECAY linearly, not the squared length curve: the
        // squared one back-loads everything into the top of the knob, which
        // left the middle settings with a strong wash and no transient.
        d->attack_gain = (int32_t)(60.0f + dec * 185.0f);
        // Softened onset. Every other voice here starts at full amplitude on
        // the first sample, which is what makes a hit read as percussive. A
        // ride wants a few ms of rise; a closed hat still starts instantly.
        uint32_t ramp_n = (uint32_t)(len * 0.009f * (float)SR);
        d->ramp = ramp_n ? 0 : ENV_ONE;
        d->ramp_step = ramp_n ? (ENV_ONE / ramp_n) : ENV_ONE;
        // The wash has to give the transient room. Two things fight it: a long
        // ride's envelope is still near full scale a few ms in, and its lower
        // filter corner passes far more of the noise than a closed hat's does,
        // so the body clips and swallows the ping. Pulling the body well down
        // fixes both - a cymbal's sustain sits far below its attack anyway.
        // Keep a real wash under the ping: dropping the body as far as the
        // transient would allow leaves a thin tick with a tail, which is
        // exactly what a ride should not sound like.
        d->body_gain = (int32_t)(256.0f - dec * 190.0f);
        break;
    }
    }
    d->active = true;
}

void drumsynth_trigger(int voice, drum_t drum, int tune, int decay) {
    drumsynth_trigger_velocity(voice, drum, tune, decay, 256);
}

bool drumsynth_active(int voice) { return dv[voice].active; }

// Interpolated table read. At low frequencies the index advances only a
// fraction of an entry per sample, so reading it raw gives a staircase whose
// step edges land high in the audio band.
static inline int32_t sine_at(uint32_t ph) {
    int32_t idx = ph >> 24;
    int32_t frac = (ph >> 16) & 0xFF;
    int32_t s0 = sine_tab[idx];
    int32_t s1 = sine_tab[(idx + 1) & 255];
    return s0 + (((s1 - s0) * frac) >> 8);
}

// One-pole high-pass, Q8 coefficient. Strips body so metal stays metallic.
static inline int32_t hp(dvoice_t *d, int32_t x, int32_t a_q8) {
    int32_t y = (a_q8 * (d->hp_prev_y + x - d->hp_prev_x)) >> 8;
    d->hp_prev_x = x;
    d->hp_prev_y = y;
    return y;
}

// Second pole, cascaded for a 12 dB/oct slope on the hat
static inline int32_t hp2(dvoice_t *d, int32_t x, int32_t a_q8) {
    int32_t y = (a_q8 * (d->hp2_prev_y + x - d->hp2_prev_x)) >> 8;
    d->hp2_prev_x = x;
    d->hp2_prev_y = y;
    return y;
}

int __not_in_flash_func(drumsynth_tick)(int voice) {
    dvoice_t *d = &dv[voice];
    if (!d->active) return 0;

    int32_t out = 0;

    switch (d->drum) {
    case DRUM_KICK: {
        d->penv = (d->penv * d->penv_coef) >> 16;
        uint32_t inc = d->pinc_end + ((d->pinc_span >> 8) * (d->penv >> 8));
        d->ph += inc;
        // Interpolated table read. At the kick's ~47 Hz the index advances
        // only about half an entry per sample, so reading the table raw gives
        // a staircase whose step edges land high in the audio band - and a
        // long DECAY sustains that stepping long enough to hear it.
        int32_t tone = sine_at(d->ph) * (int32_t)(d->env >> 8) >> 8;
        // A kick is a sine: anything above a few hundred hertz in its tail is
        // an artifact, whatever produced it. One pole at ~800 Hz guarantees a
        // clean tail without dulling the sweep, which starts around 210 Hz.
        d->lp_y += ((tone - d->lp_y) * 60) >> 8;
        out = d->lp_y;
        if (d->click) { // sharp transient, deliberately bypassing the filter
            d->click--;
            int32_t c = (int32_t)d->click;
            out += (c * c) >> 2; // peaks near full scale without hard clipping
            out += (noise() * c) >> 8;
        }
        break;
    }
    case DRUM_SNARE: {
        d->penv = (d->penv * d->penv_coef) >> 16;
        uint32_t inc = d->pinc_end + ((d->pinc_span >> 8) * (d->penv >> 8));
        d->ph += inc;
        int32_t body = sine_at(d->ph) * (int32_t)(d->env >> 8) >> 8;

        d->nenv = (d->nenv * d->nenv_coef) >> 16;
        int32_t wires = noise() * (int32_t)(d->nenv >> 8) >> 8;
        // Band-limit the rattle. Full-range white noise IS a hi-hat: unfiltered
        // the snare had half its energy above 4 kHz, against 93% for the actual
        // hat. Two poles near 5 kHz bring that to about a third, which leaves
        // the body audible and keeps the snap - filtering harder sounded
        // muffled. The high-pass keeps the rattle out of the body's register.
        d->lp_y += ((wires - d->lp_y) * 205) >> 8;
        d->lp2_y += ((d->lp_y - d->lp2_y) * 205) >> 8;
        wires = hp(d, d->lp2_y, 230);

        // The body deliberately keeps its low end: no high-pass on it, or the
        // thump goes with it.
        out = (body * 10 + wires * 14) >> 4;
        // Balance: about half the energy under 400 Hz so the thump leads,
        // with enough rattle left that it is still a snare and not a tom.
        out = (out * 5) >> 2; // modest drive
        break;
    }
    default: {
        // white noise through two one-pole high-passes (12 dB/oct). Gain up
        // first: high-passing noise this hard throws most of its energy away.
        int32_t n = noise() * 6;
        n = hp(d, n, d->hp_a);
        n = hp2(d, n, d->hp_a);
        out = ((n * (int32_t)(d->env >> 8) >> 8) * d->body_gain) >> 8;
        if (d->aenv > 32) { // stick attack rides on top of the first few ms
            d->aenv = (d->aenv * d->aenv_coef) >> 16;
            int32_t a = (noise() * (int32_t)(d->aenv >> 8)) >> 8;
            out += (a * d->attack_gain) >> 8;
        }
        if (d->ramp < ENV_ONE) { // soften the onset for long decays
            d->ramp += d->ramp_step;
            if (d->ramp > ENV_ONE) d->ramp = ENV_ONE;
            out = (out * (int32_t)(d->ramp >> 8)) >> 8;
        }
        break;
    }
    }

    d->env = (d->env * d->env_coef) >> 16;
    // The snare's tone envelope is much shorter than its noise tail, so the
    // voice must stay alive until BOTH have decayed or the wires get cut off.
    bool quiet = d->env < 24 &&
                 (d->drum != DRUM_SNARE || d->nenv < 24);
    if (quiet) {
        d->active = false;
        d->env = 0;
    }

    if (out > SYNTH_FULL) out = SYNTH_FULL;
    if (out < -SYNTH_FULL) out = -SYNTH_FULL;
    out = (out * d->velocity) >> 8;
    return out;
}
