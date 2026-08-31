// =============================================================================
// DRUMS - sample playback drum machine
// V1: kick - V2: snare - V3: hat, one drum per output jack.
// Both drum modes read the same Truchets banks. BANK selects OG Grids,
// Electronic or Breakbeat; X/Y morph smoothly through each 5x5 map.
// CV1 remains a live FILL/DENSITY control: noon is a stable core groove,
// below noon thins lower-priority hits and above noon adds probabilistic
// bank-informed fills, increasingly favouring the end of the 32-step phrase.
// =============================================================================
#include <stdio.h>

#include "../config.h"
#include "../drum_timing.h"
#include "../gfx.h"
#include "../mode.h"
#include "../rng.h"
#include "../samples.h"
#include "../samples_bank1.h"
#include "../samples_bank2.h"
#include "../sampler.h"
#include "../truchet_patterns.h"
#include "mode_util.h"

static int16_t p_bank = TRUCHET_DEFAULT_BANK;
static int16_t p_kit = 0;
static int16_t p_x = TRUCHET_DEFAULT_X;
static int16_t p_y = TRUCHET_DEFAULT_Y;
static int16_t p_swing = 50; // 50% = straight timing
static int16_t p_tune = 0;  // kick pitch in semitones

static void fmt_semi(char *out, int16_t v) { sprintf(out, "%+d", v); }
static void fmt_pct(char *out, int16_t v) { sprintf(out, "%d%%", v); }
static void fmt_kit(char *out, int16_t v) {
    static const char *const NAMES[3] = {"909", "BANK 1", "BANK 2"};
    sprintf(out, "%s", NAMES[v]);
}
static void fmt_bank(char *out, int16_t v) {
    sprintf(out, "%s", truchet_bank_name(v));
}

static const param_t PARAMS[] = {
    {"BANK", &p_bank, 0, TRUCHET_BANKS - 1, 1, fmt_bank},
    {"KIT", &p_kit, 0, 2, 1, fmt_kit},
    {"X", &p_x, 0, 100, 5, 0},
    {"Y", &p_y, 0, 100, 5, 0},
    {"SWING", &p_swing, 50, 75, 1, fmt_pct},
    {"TUNE", &p_tune, -12, 12, 1, fmt_semi},
};

_Static_assert(BANK1_SAMPLE_RATE_HZ == AUDIO_SR,
               "sample bank 1 has the wrong sample rate");
_Static_assert(BANK2_SAMPLE_RATE_HZ == AUDIO_SR,
               "sample bank 2 has the wrong sample rate");

typedef struct {
    const uint8_t *pcm[3];
    uint32_t len[3];
} sample_kit_t;

static const sample_kit_t KITS[3] = {
    {{KICK_PCM, SNARE_PCM, HAT_PCM}, {KICK_LEN, SNARE_LEN, HAT_LEN}},
    {{BANK1_KICK_PCM, BANK1_SNARE_PCM, BANK1_HAT_PCM},
     {BANK1_KICK_LEN, BANK1_SNARE_LEN, BANK1_HAT_LEN}},
    {{BANK2_KICK_PCM, BANK2_SNARE_PCM, BANK2_HAT_PCM},
     {BANK2_KICK_LEN, BANK2_SNARE_LEN, BANK2_HAT_LEN}},
};

static uint32_t planned_mask[3];
static int16_t planned_bank = -1, planned_x = -1, planned_y = -1;
static int planned_fill = -1;
static int pos;
static uint8_t lit[3]; // decaying display brightness per drum
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

// Roll every random choice before playback so the OLED always shows the
// phrase that is going to play. Replanning mid-phrase only changes steps at
// and after the playhead; steps already played are left alone.
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
        // A direct BANK/X/Y edit supersedes a queued random jump.
        // Restore the wrapped first bar too if it had already been pre-rolled.
        if (xy_queued && queued_at == 0 && pos >= 16)
            plan_range(0, 16, p_bank, p_x, p_y);
        xy_queued = false;
        plan_from(pos);
    } else if (fill_changed && xy_queued) {
        // Keep a queued X/Y jump alive while the fill knob moves: update the
        // rest of this bar with the current coordinates and re-roll the next
        // bar with the queued ones.
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

    // Pre-roll only the next bar. This updates the OLED immediately without
    // touching any step that still belongs to the currently playing bar.
    plan_range(queued_at, queued_at + 16, p_bank, queued_x, queued_y);
}

static void apply_queued_xy(void) {
    if (!xy_queued || pos != queued_at) return;
    p_x = queued_x;
    p_y = queued_y;
    xy_queued = false;

    // At a wrapped boundary, the second bar occupied cells that were still
    // playing when the request was queued. It is safe to pre-roll them now.
    if (pos == 0)
        plan_range(16, TRUCHET_STEPS, p_bank, p_x, p_y);
    remember_current_plan();
}

static uint16_t hat_velocity(void) {
    int fill = fill_percent();
    return fill > 60 ? drum_hat_velocity_q8(fill, rng_u32()) : 256;
}

static void fire_hits(uint32_t hits) {
    const sample_kit_t *kit = &KITS[p_kit];
    for (int d = 0; d < 3; d++) {
        if (!((hits >> d) & 1u)) continue;
        uint16_t velocity = d == 2 ? hat_velocity() : 256;
        sampler_trigger(d, kit->pcm[d], kit->len[d], d == 0 ? p_tune : 0,
                        velocity);
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
        sampler_set_synth(v, false);
        lit[v] = 0;
    }
}

static void on_tick(uint64_t now) {
    // A stalled main loop must never let a delayed offbeat overtake the next
    // clock tick. Normally update() has already fired it well before here.
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
    const int n = TRUCHET_STEPS;
    int w = 112 / n;
    if (w > 8) w = 8;
    if (w < 2) w = 2;

    for (int d = 0; d < 3; d++) {
        // Rows start below the UI header (which owns y 0-10) and are packed
        // tightly enough to leave a line for the FILL readout underneath.
        int y = 13 + d * 11;
        // label, inverted while the drum is sounding
        if (lit[d] > 40) {
            gfx_fill_rect(0, y - 1, 9, 10, 1);
            gfx_text(2, y, LABEL[d], 1);
            // punch the letter back out of the block
            for (int px = 2; px < 8; px++)
                for (int py = y; py < y + 8; py++)
                    if ((px + py) & 1) gfx_pixel(px, py, 0);
        } else {
            gfx_text(2, y, LABEL[d], 1);
        }

        for (int i = 0; i < n; i++) {
            int x = 12 + i * w;
            if ((planned_mask[d] >> i) & 1u)
                gfx_fill_rect(x, y, w - 1, 7, 1);
            else
                gfx_hline(x, y + 6, w - 1, 1);
        }
        // playhead
        int px = 12 + pos * w;
        gfx_rect(px - 1, y - 1, w + 1, 9, 1);
    }

}

// FILL is a live performance control with no other readout, so it rides in
// the header. BANK/X/Y are available in the parameter line.
static void header(char *out) { sprintf(out, "f%d%%", (int)(g.cv1 * 100)); }

const mode_t MODE_DRUMS = {
    .name = "DRUMS",
    .reset = reset,
    .on_tick = on_tick,
    .update = update,
    .draw = draw,
    .header = header,
    .restart = restart,
    .randomize = randomize_xy,
    .params = PARAMS,
    .n_params = 6,
    .hide_globals = true,
};
