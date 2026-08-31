#include "ui.h"

#include <stdio.h>
#include <string.h>

#include "pico/flash.h"
#include "pico/time.h"

#include "clockgen.h"
#include "gfx.h"
#include "mode.h"
#include "scales.h"
#include "ssd1306.h"
#include "state.h"
#include "voice.h"

#define FRAME_MS 66 // ~15 FPS

static void draw_centered(const char *text, int y, int size) {
    int width = (int)strlen(text) * 6 * size;
    int x = (OLED_W - width) / 2;
    if (x < 0) x = 0;
    gfx_text(x, y, text, size);
}

static void draw_header(const mode_t *m) {
    gfx_text(0, 1, m->name, 1);
    char info[20];
    if (clock_is_external()) {
        int f = clock_factor();
        if (f < 0)
            sprintf(info, "EXT/%d", -f);
        else if (f > 1)
            sprintf(info, "EXTx%d", f);
        else
            sprintf(info, "EXT");
    } else {
        sprintf(info, "%d", (int)(clock_bpm() + 0.5f));
    }
    int info_w = 6 * (int)strlen(info);
    gfx_text(OLED_W - info_w, 1, info, 1);

    if (m->header) {
        char extra[12];
        extra[0] = 0;
        m->header(extra);
        int name_w = 6 * (int)strlen(m->name);
        int x = name_w + 8;                       // gap after the mode name
        int room = (OLED_W - info_w - 4) - x;     // stop clear of the clock
        if ((int)strlen(extra) * 6 <= room) gfx_text(x, 1, extra, 1);
    }
    gfx_hline(0, 10, OLED_W, 1);
}

static void draw_param_line(void) {
    gfx_hline(0, 53, OLED_W, 1);
    char line[24];
    if (g.param_idx == 0) {
        sprintf(line, "<MODE %d/%d>", g.mode_idx + 1, NUM_MODES);
    } else {
        const param_t *p = param_for_slot(g.param_idx);
        if (!p) return;
        char val[12];
        if (p->fmt)
            p->fmt(val, *p->value);
        else
            sprintf(val, "%d", (int)*p->value);
        sprintf(line, "<%s %s>", p->name, val);
    }
    gfx_text(0, 55, line, 1);
}

static void draw_summary(const mode_t *m) {
    char buf[24], nn[8];
    gfx_text(28, 0, "= STATUS =", 1);
    sprintf(buf, "Mode: %s", m->name);
    gfx_text(0, 12, buf, 1);
    sprintf(buf, "Clock: %s %d bpm", clock_is_external() ? "EXT" : "INT",
            (int)(clock_bpm() + 0.5f));
    gfx_text(0, 22, buf, 1);
    note_name(base_midi(), nn);
    sprintf(buf, "Root: %s  Gate:%d%%", nn, g.gate_pct);
    gfx_text(0, 32, buf, 1);
    for (int v = 0; v < 3; v++) {
        float f = voice_current_freq(v);
        if (f > 0.0f)
            sprintf(buf, "V%d:%dHz", v + 1, (int)f);
        else
            sprintf(buf, "V%d:--", v + 1);
        gfx_text(v * 44, 42, buf, 1);
    }
    // CLK IN diagnostics: live voltage, detected swing, trigger state.
    // If a patched clock shows swing below ~0.2 V the jack is attenuating it
    // too far for any threshold to work - that is a wiring problem, not code.
    sprintf(buf, "CLKIN %.2fV ~%.2f %s", (double)clock_in_volts(),
            (double)clock_in_swing_volts(), clock_in_level() ? "HI" : "lo");
    gfx_text(0, 54, buf, 1);
}

static void draw_splash(void) {
    gfx_clear();
    gfx_text(22, 14, "TECLA", 2);
    gfx_text(16, 34, "BASS ENGINE", 1);
    gfx_text(46, 48, "v3.0", 1);
    gfx_show();
    sleep_ms(900);
}

void ui_core1_entry(void) {
    // allow core 0 to pause us safely during flash writes
    flash_safe_execute_core_init();

    ssd1306_init();
    draw_splash();

    while (true) {
        uint64_t t0 = time_us_64();
        gfx_clear();
        const mode_t *m = MODES[g.mode_idx];
        if (g.toast_until && t0 < g.toast_until) {
            if (g.toast_detail[0]) {
                draw_centered(g.toast, 15, 2);
                draw_centered(g.toast_detail, 36, 2);
            } else {
                draw_centered(g.toast, 26, 2);
            }
        } else if (g.show_summary) {
            draw_summary(m);
        } else {
            draw_header(m);
            m->draw();
            draw_param_line();
        }
        gfx_show();

        int32_t left = FRAME_MS - (int32_t)((time_us_64() - t0) / 1000);
        if (left > 0) sleep_ms((uint32_t)left);
    }
}
