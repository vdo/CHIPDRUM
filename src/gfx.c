#include "gfx.h"

#include <stdlib.h>

#include "font5x8.h"

uint8_t gfx_fb[OLED_FB_SIZE];

void gfx_clear(void) {
    for (int i = 0; i < OLED_FB_SIZE; i++) gfx_fb[i] = 0;
}

void gfx_pixel(int x, int y, int on) {
    if (x < 0 || x >= OLED_W || y < 0 || y >= OLED_H) return;
    int idx = x + (y / 8) * OLED_W;
    uint8_t bit = 1u << (y & 7);
    if (on)
        gfx_fb[idx] |= bit;
    else
        gfx_fb[idx] &= ~bit;
}

void gfx_hline(int x, int y, int w, int on) {
    for (int i = 0; i < w; i++) gfx_pixel(x + i, y, on);
}

void gfx_vline(int x, int y, int h, int on) {
    for (int i = 0; i < h; i++) gfx_pixel(x, y + i, on);
}

void gfx_line(int x0, int y0, int x1, int y1, int on) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        gfx_pixel(x0, y0, on);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_rect(int x, int y, int w, int h, int on) {
    gfx_hline(x, y, w, on);
    gfx_hline(x, y + h - 1, w, on);
    gfx_vline(x, y, h, on);
    gfx_vline(x + w - 1, y, h, on);
}

void gfx_fill_rect(int x, int y, int w, int h, int on) {
    for (int j = 0; j < h; j++) gfx_hline(x, y + j, w, on);
}

void gfx_circle(int cx, int cy, int r, int on) {
    int x = r, y = 0, err = 0;
    while (x >= y) {
        gfx_pixel(cx + x, cy + y, on); gfx_pixel(cx + y, cy + x, on);
        gfx_pixel(cx - y, cy + x, on); gfx_pixel(cx - x, cy + y, on);
        gfx_pixel(cx - x, cy - y, on); gfx_pixel(cx - y, cy - x, on);
        gfx_pixel(cx + y, cy - x, on); gfx_pixel(cx + x, cy - y, on);
        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) { x--; err += 1 - 2 * x; }
    }
}

void gfx_text(int x, int y, const char *s, int size) {
    for (; *s; s++) {
        char c = *s;
        if (c < FONT_FIRST || c > FONT_LAST) c = '?';
        const uint8_t *glyph = font5x8[c - FONT_FIRST];
        for (int col = 0; col < FONT_W; col++) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < FONT_H; row++) {
                if (bits & (1u << row)) {
                    if (size == 1)
                        gfx_pixel(x + col, y + row, 1);
                    else
                        gfx_fill_rect(x + col * size, y + row * size, size,
                                      size, 1);
                }
            }
        }
        x += (FONT_W + 1) * size;
    }
}

void gfx_show(void) { ssd1306_show(gfx_fb); }
