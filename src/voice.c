#include "voice.h"

#include <math.h>

#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "config.h"

typedef struct {
    uint pin;
    uint slice;
    uint chan;
    float freq;        // current pitch (after glide)
    float target;      // target pitch
    float glide_ms;    // 0 = instant
    bool gate;
    uint64_t gate_off_at; // 0 = no scheduled off
    uint8_t duty_pct;
    // percussive sweep
    bool sweep;
    float sweep_from, sweep_to, sweep_tau_ms;
    uint64_t sweep_start;
    uint16_t last_wrap;
} voice_t;

static voice_t voices[NUM_VOICES];
static const uint PINS[NUM_VOICES] = {PIN_VOICE1, PIN_VOICE2, PIN_VOICE3};
static bool sample_mode[NUM_VOICES];

static void write_hw(voice_t *vc) {
    float f = vc->freq;
    if (f < VOICE_FREQ_MIN) f = VOICE_FREQ_MIN;
    if (f > VOICE_FREQ_MAX) f = VOICE_FREQ_MAX;
    uint32_t wrap = (uint32_t)(PWM_COUNT_HZ / f) - 1;
    if (wrap > 65535) wrap = 65535;
    if (wrap < 15) wrap = 15;
    vc->last_wrap = (uint16_t)wrap;
    pwm_set_wrap(vc->slice, (uint16_t)wrap);
    uint16_t level = 0;
    if (vc->gate) level = (uint16_t)((wrap * vc->duty_pct) / 100u);
    pwm_set_chan_level(vc->slice, vc->chan, level);
}

void voice_init_all(void) {
    for (int i = 0; i < NUM_VOICES; i++) {
        voice_t *vc = &voices[i];
        vc->pin = PINS[i];
        vc->slice = pwm_gpio_to_slice_num(vc->pin);
        vc->chan = pwm_gpio_to_channel(vc->pin);
        vc->freq = vc->target = 55.0f;
        vc->glide_ms = 0.0f;
        vc->gate = false;
        vc->duty_pct = 50;
        gpio_set_function(vc->pin, GPIO_FUNC_PWM);
        pwm_set_clkdiv(vc->slice, PWM_CLKDIV);
        write_hw(vc);
        pwm_set_enabled(vc->slice, true);
    }
}

void voice_freq(int v, float hz) { voices[v].target = hz; }

void voice_glide(int v, float ms) { voices[v].glide_ms = ms; }

void voice_duty(int v, uint8_t pct) {
    if (pct < 1) pct = 1;
    if (pct > 99) pct = 99;
    voices[v].duty_pct = pct;
}

void voice_gate(int v, bool on) {
    voices[v].gate = on;
    voices[v].gate_off_at = 0;
    write_hw(&voices[v]);
}

void voice_gate_ms(int v, uint32_t ms, uint64_t now) {
    voices[v].gate = true;
    voices[v].gate_off_at = ms ? now + (uint64_t)ms * 1000 : 0;
    write_hw(&voices[v]);
}

void voice_sweep(int v, float f0, float f1, float tau_ms, uint32_t gate_ms,
                 uint64_t now) {
    voice_t *vc = &voices[v];
    vc->sweep = true;
    vc->sweep_from = f0;
    vc->sweep_to = f1;
    vc->sweep_tau_ms = tau_ms > 1.0f ? tau_ms : 1.0f;
    vc->sweep_start = now;
    vc->freq = vc->target = f0;
    voice_gate_ms(v, gate_ms, now);
}

void voice_update(uint64_t now_us, float dt_s) {
    for (int i = 0; i < NUM_VOICES; i++) {
        if (sample_mode[i]) continue; // the sampler owns this slice
        voice_t *vc = &voices[i];
        bool dirty = false;

        if (vc->gate && vc->gate_off_at && now_us >= vc->gate_off_at) {
            vc->gate = false;
            vc->gate_off_at = 0;
            vc->sweep = false;
            dirty = true;
        }

        if (vc->sweep) {
            float t_ms = (now_us - vc->sweep_start) * 0.001f;
            float f = vc->sweep_to +
                      (vc->sweep_from - vc->sweep_to) * expf(-t_ms / vc->sweep_tau_ms);
            if (fabsf(f - vc->freq) > 0.05f) {
                vc->freq = vc->target = f;
                dirty = true;
            }
        } else if (vc->freq != vc->target) {
            if (vc->glide_ms <= 0.0f) {
                vc->freq = vc->target;
            } else {
                float k = dt_s * 1000.0f / vc->glide_ms;
                if (k > 1.0f) k = 1.0f;
                vc->freq += (vc->target - vc->freq) * k;
                if (fabsf(vc->freq - vc->target) < 0.05f) vc->freq = vc->target;
            }
            dirty = true;
        }

        if (dirty) write_hw(vc);
    }
}

void voice_set_sample_mode(int v, bool on) {
    sample_mode[v] = on;
    if (on) return;
    // restore square-oscillator operation: the sampler left the slice at
    // clkdiv 1 / wrap 255
    voice_t *vc = &voices[v];
    vc->gate = false;
    vc->sweep = false;
    vc->gate_off_at = 0;
    pwm_set_enabled(vc->slice, false);
    pwm_set_clkdiv(vc->slice, PWM_CLKDIV);
    write_hw(vc);
    pwm_set_enabled(vc->slice, true);
}

float voice_current_freq(int v) {
    return voices[v].gate ? voices[v].freq : 0.0f;
}

bool voice_is_gated(int v) { return voices[v].gate; }
