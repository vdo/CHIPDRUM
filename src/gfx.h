#pragma once
// Framebuffer graphics primitives for the SSD1306 (page layout)
#include <stdint.h>

#include "ssd1306.h"

extern uint8_t gfx_fb[OLED_FB_SIZE];

void gfx_clear(void);
void gfx_pixel(int x, int y, int on);
void gfx_hline(int x, int y, int w, int on);
void gfx_vline(int x, int y, int h, int on);
void gfx_line(int x0, int y0, int x1, int y1, int on);
void gfx_rect(int x, int y, int w, int h, int on);
void gfx_fill_rect(int x, int y, int w, int h, int on);
void gfx_circle(int cx, int cy, int r, int on);
// 5x8 font; size 1 or 2
void gfx_text(int x, int y, const char *s, int size);
void gfx_show(void);
