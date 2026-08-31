#include "sampler.h"

#include <math.h>

#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "hardware/timer.h"

#include "config.h"
#include "drumsynth.h"
#include "samples.h"
#include "voice.h"

#define ALARM_NUM 1
#define ALARM_IRQ TIMER_IRQ_1
#define PERIOD_US AUDIO_PERIOD_US

// A stale samples.h would play back at the wrong pitch, silently. Catch it at
// compile time rather than wondering why the kit sounds detuned.
_Static_assert(SAMPLE_RATE_HZ == AUDIO_SR,
               "samples.h was generated at a different rate - rerun "
               "tools/make_samples.sh");

typedef struct {
    const uint8_t *pcm;
    uint32_t len;
    uint32_t pos;   // 16.16 fixed point position into pcm
    uint32_t step;  // 16.16 increment; 1<<16 = native pitch
    uint16_t velocity; // Q8 amplitude, 256 = unity
    bool playing;
    bool active;    // voice is in DAC mode
    bool synth;     // render from drumsynth instead of PCM
    uint slice;
    uint chan;
} splay_t;

static splay_t sp[NUM_VOICES];
static const uint8_t PINS[NUM_VOICES] = {PIN_VOICE1, PIN_VOICE2, PIN_VOICE3};

static uint32_t next_at; // absolute time of the next sample tick

// Hot path: keep it short, this runs AUDIO_SR times a second.
static void __not_in_flash_func(alarm_isr)(void) {
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
    // Re-arm from the previous target, not from "now": scheduling off the
    // current time lets interrupt latency accumulate as sample-clock jitter.
    next_at += PERIOD_US;
    if ((int32_t)(next_at - timer_hw->timerawl) < 0)
        next_at = timer_hw->timerawl + PERIOD_US; // fell behind, resync
    timer_hw->alarm[ALARM_NUM] = next_at;

    for (int v = 0; v < NUM_VOICES; v++) {
        splay_t *s = &sp[v];
        if (!s->active) continue;
        // idle at mid-scale so nothing steps the output when silent
        uint16_t level = DAC_FROM_PCM8(128);
        if (s->synth) {
            level = DAC_FROM_SYNTH(drumsynth_tick(v));
        } else if (s->playing) {
            uint32_t idx = s->pos >> 16;
            if (idx >= s->len) {
                s->playing = false;
            } else {
                int32_t centered = (int32_t)s->pcm[idx] - 128;
                centered = (centered * s->velocity) >> 8;
                level = DAC_FROM_PCM8((uint8_t)(centered + 128));
                s->pos += s->step;
            }
        }
        pwm_set_chan_level(s->slice, s->chan, level);
    }
}

void sampler_init(void) {
    for (int v = 0; v < NUM_VOICES; v++) {
        sp[v].slice = pwm_gpio_to_slice_num(PINS[v]);
        sp[v].chan = pwm_gpio_to_channel(PINS[v]);
        sp[v].active = false;
        sp[v].playing = false;
    }
    // Claim the alarm so a clash with the SDK's alarm pool panics at boot
    // instead of silently corrupting timing later.
    hardware_alarm_claim(ALARM_NUM);
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    irq_set_exclusive_handler(ALARM_IRQ, alarm_isr);
    irq_set_enabled(ALARM_IRQ, true);
    next_at = timer_hw->timerawl + PERIOD_US;
    timer_hw->alarm[ALARM_NUM] = next_at;
}

void sampler_enable(int voice, bool on) {
    splay_t *s = &sp[voice];
    if (s->active == on) return;
    s->active = on;
    s->playing = false;
    if (!on) s->synth = false;
    if (on) {
        voice_set_sample_mode(voice, true); // stop voice_update touching it
        // 8-bit DAC locked to the sample clock (see AUDIO_SR in config.h)
        pwm_set_enabled(s->slice, false);
        pwm_set_clkdiv(s->slice, 1.0f);
        pwm_set_wrap(s->slice, DAC_WRAP);
        pwm_set_chan_level(s->slice, s->chan, DAC_FROM_PCM8(128));
        pwm_set_enabled(s->slice, true);
    }
    voice_set_sample_mode(voice, on); // also hands the slice back when off
}

bool sampler_enabled(int voice) { return sp[voice].active; }

void sampler_set_synth(int voice, bool on) {
    sp[voice].synth = on;
    sp[voice].playing = false;
}

void sampler_trigger(int voice, const uint8_t *pcm, uint32_t len,
                     int semitones, uint16_t velocity_q8) {
    splay_t *s = &sp[voice];
    if (!s->active || len == 0) return;
    float rate = powf(2.0f, semitones / 12.0f);
    uint32_t step = (uint32_t)(rate * 65536.0f);
    if (step < 4096) step = 4096;          // floor: 4 octaves down
    if (step > 16u << 16) step = 16u << 16; // ceiling: 4 octaves up
    if (velocity_q8 > 256) velocity_q8 = 256;
    // The ISR reads these fields; updating them piecemeal could hand it a new
    // buffer with the previous length and read out of bounds.
    uint32_t irq = save_and_disable_interrupts();
    s->pcm = pcm;
    s->len = len;
    s->step = step;
    s->velocity = velocity_q8;
    s->pos = 0;
    s->playing = true;
    restore_interrupts(irq);
}

bool sampler_playing(int voice) { return sp[voice].playing; }

uint8_t sampler_progress(int voice) {
    splay_t *s = &sp[voice];
    if (!s->playing || s->len == 0) return 0;
    uint32_t idx = s->pos >> 16;
    if (idx >= s->len) return 255;
    return (uint8_t)((idx * 255u) / s->len);
}
