// =============================================================================
// SYNDRUM - synthesized drums, 909-flavoured with an industrial snare
// V1: kick - V2: snare - V3: hat, one drum per output jack.
// Same Truchets sequencing as DRUMS so the two can be A/B'd on identical
// patterns; the difference is entirely in the sound engine. Nothing is
// sampled here - every hit is generated live, so TUNE and DECAY reach places
// a fixed sample cannot.
// =============================================================================
#include <stdio.h>

#include "../drum_timing.h"
#include "../drumsynth.h"
#include "../gfx.h"
#include "../mode.h"
#include "../rng.h"
#include "../sampler.h"
#include "../truchet_patterns.h"
#include "mode_util.h"

static int16_t p_bank = TRUCHET_DEFAULT_BANK;
static int16_t p_x = TRUCHET_DEFAULT_X;
static int16_t p_y = TRUCHET_DEFAULT_Y;
static int16_t p_swing = 50;
static int16_t p_ktune = 0, p_kdecay = 55;
static int16_t p_stune = 0, p_sdecay = 45;
static int16_t p_htune = 0, p_hdecay = 30;

static void fmt_semi(char *out, int16_t v) { sprintf(out, "%+d", v); }
static void fmt_pct(char *out, int16_t v) { sprintf(out, "%d%%", v); }
static void fmt_bank(char *out, int16_t v) {
    sprintf(out, "%s", truchet_bank_name(v));
}

static const param_t PARAMS[] = {
    {"BANK", &p_bank, 0, TRUCHET_BANKS - 1, 1, fmt_bank},
    {"X", &p_x, 0, 100, 5, 0},
    {"Y", &p_y, 0, 100, 5, 0},
    {"SWING", &p_swing, 50, 75, 1, fmt_pct},
    {"K TUNE", &p_ktune, -24, 24, 1, fmt_semi},
    {"K DECAY", &p_kdecay, 0, 100, 5, fmt_pct},
    {"S TUNE", &p_stune, -24, 24, 1, fmt_semi},
    {"S DECAY", &p_sdecay, 0, 100, 5, fmt_pct},
    {"H TUNE", &p_htune, -24, 24, 1, fmt_semi},
    {"H DECAY", &p_hdecay, 0, 100, 5, fmt_pct},
};

static uint32_t planned_mask[3];
static int16_t planned_bank = -1, planned_x = -1, planned_y = -1;
static int planned_fill = -1;
static int pos;
static uint8_t lit[3];
static bool xy_queued;
static int16_t queued_x, queued_y;
static int queued_at;
static uint32_t delayed_hits;
static uint64_t delayed_at;

static int fill_percent(void) {
    int pct = (int)(g.cv1 * 100.0f + 0.5f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

static void plan_range(int first_step, int end_step, int16_t bank, int16_t x,
                       int16_t y) {
    if (first_step < 0) first_step = 0;
    if (first_step >= TRUCHET_STEPS) first_step = TRUCHET_STEPS - 1;
    if (end_step > TRUCHET_STEPS) end_step = TRUCHET_STEPS;
    for (int d = 0; d < 3; d++) {
        for (int step = first_step; step < end_step; step++) {
            uint32_t bit = 1u << step;
            if (truchet_should_hit(bank, x, y, d, step, g.cv1,
                                   rng_f()))
                planned_mask[d] |= bit;
            else
                planned_mask[d] &= ~bit;
        }
    }
}

static void remember_current_plan(void) {
    planned_bank = p_bank;
    planned_x = p_x;
    planned_y = p_y;
    planned_fill = fill_percent();
}

static void plan_from(int first_step) {
    plan_range(first_step, TRUCHET_STEPS, p_bank, p_x, p_y);
    remember_current_plan();
}

static void ensure_plan(void) {
    bool pattern_changed = planned_bank != p_bank || planned_x != p_x ||
                           planned_y != p_y;
    bool fill_changed = planned_fill != fill_percent();
    if (pattern_changed) {
        if (xy_queued && queued_at == 0 && pos >= 16)
            plan_range(0, 16, p_bank, p_x, p_y);
        xy_queued = false;
        plan_from(pos);
    } else if (fill_changed && xy_queued) {
        if (queued_at == 16) {
            plan_range(pos, 16, p_bank, p_x, p_y);
            plan_range(16, 32, p_bank, queued_x, queued_y);
        } else {
            plan_range(pos, 32, p_bank, p_x, p_y);
            plan_range(0, 16, p_bank, queued_x, queued_y);
        }
        planned_fill = fill_percent();
    } else if (fill_changed) {
        plan_from(pos);
    }
}

static void randomize_xy(int16_t *x, int16_t *y) {
    queued_x = (int16_t)(rng_below(21) * 5);
    queued_y = (int16_t)(rng_below(21) * 5);
    *x = queued_x;
    *y = queued_y;
    queued_at = drum_next_bar_boundary(pos);
    xy_queued = true;
    plan_range(queued_at, queued_at + 16, p_bank, queued_x, queued_y);
}

static void apply_queued_xy(void) {
    if (!xy_queued || pos != queued_at) return;
    p_x = queued_x;
    p_y = queued_y;
    xy_queued = false;
    if (pos == 0)
        plan_range(16, TRUCHET_STEPS, p_bank, p_x, p_y);
    remember_current_plan();
}

static uint16_t hat_velocity(void) {
    int fill = fill_percent();
    return fill > 60 ? drum_hat_velocity_q8(fill, rng_u32()) : 256;
}

static void fire_hits(uint32_t hits) {
    for (int d = 0; d < 3; d++) {
        if (!((hits >> d) & 1u)) continue;

        uint16_t velocity = 256;
        switch (d) {
        case 0:
            drumsynth_trigger(0, DRUM_KICK, p_ktune, p_kdecay);
            break;
        case 1:
            drumsynth_trigger(1, DRUM_SNARE, p_stune, p_sdecay);
            break;
        default:
            velocity = hat_velocity();
            drumsynth_trigger_velocity(
                2, DRUM_HAT, p_htune, p_hdecay, velocity);
            break;
        }
        lit[d] = (uint8_t)(velocity - 1u);
    }
}

static void restart(void) {
    pos = 0;
    delayed_hits = 0;
    xy_queued = false;
    plan_from(0);
}

static void reset(void) {
    restart();
    for (int v = 0; v < 3; v++) {
        sampler_enable(v, true);
        sampler_set_synth(v, true);
        lit[v] = 0;
    }
}

static void on_tick(uint64_t now) {
    if (delayed_hits) {
        fire_hits(delayed_hits);
        delayed_hits = 0;
    }
    ensure_plan();
    apply_queued_xy();
    if (pos >= TRUCHET_STEPS) pos = 0;

    uint32_t hits = 0;
    for (int d = 0; d < 3; d++) {
        if ((planned_mask[d] >> pos) & 1u) hits |= 1u << d;
    }
    uint32_t delay = drum_swing_delay_us(p_swing, clock_period_us(), pos);
    if (hits && delay) {
        delayed_hits = hits;
        delayed_at = now + delay;
    } else {
        fire_hits(hits);
    }
    pos = (pos + 1) % TRUCHET_STEPS;
    if (pos == 0 && !(xy_queued && queued_at == 0)) plan_from(0);
}

static void update(float dt, uint64_t now) {
    if (delayed_hits && now >= delayed_at) {
        fire_hits(delayed_hits);
        delayed_hits = 0;
    }
    int fade = (int)(dt * 900.0f);
    if (fade < 1) fade = 1;
    for (int d = 0; d < 3; d++)
        lit[d] = lit[d] > fade ? (uint8_t)(lit[d] - fade) : 0;
}

static void draw(void) {
    static const char *const LABEL[3] = {"K", "S", "H"};
    const int16_t *dec[3] = {&p_kdecay, &p_sdecay, &p_hdecay};
    const int n = TRUCHET_STEPS;
    // Match DRUMS' step size: the grid runs to x=108, leaving just enough for
    // a compact decay bar. The per-drum TUNE value lives in the parameter
    // line rather than eating a third of the screen width.
    int w = 96 / (n > 0 ? n : 16);
    if (w > 7) w = 7;
    if (w < 2) w = 2;

    for (int d = 0; d < 3; d++) {
        int y = 13 + d * 11; // clear of the UI header at y 0-10
        if (lit[d] > 40)
            gfx_fill_rect(0, y - 1, 9, 10, 1);
        gfx_text(2, y, LABEL[d], 1);

        for (int i = 0; i < n; i++) {
            int x = 12 + i * w;
            if ((planned_mask[d] >> i) & 1u)
                gfx_fill_rect(x, y, w - 1, 7, 1);
            else
                gfx_hline(x, y + 6, w - 1, 1);
        }
        gfx_rect(12 + pos * w - 1, y - 1, w + 1, 9, 1);

        // decay as a small bar, so the grid keeps the width instead. Sits
        // after the fixed 32-step grid and clamped to stay on screen.
        int bx = 12 + n * w + 2;
        if (bx > 112) bx = 112;
        gfx_rect(bx, y, 14, 7, 1);
        int fillw = (*dec[d] * 12) / 100;
        if (fillw > 0) gfx_fill_rect(bx + 1, y + 1, fillw, 5, 1);
    }
}

static void header(char *out) { sprintf(out, "f%d%%", (int)(g.cv1 * 100)); }

const mode_t MODE_SYNDRUM = {
    .name = "SYNDRUM",
    .reset = reset,
    .on_tick = on_tick,
    .update = update,
    .draw = draw,
    .params = PARAMS,
    .header = header,
    .restart = restart,
    .randomize = randomize_xy,
    .n_params = 10,
    .hide_globals = true,
};
