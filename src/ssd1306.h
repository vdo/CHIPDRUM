#pragma once
// Minimal SSD1306 128x64 I2C driver (blocking; runs on core 1)
#include <stdint.h>

#define OLED_W 128
#define OLED_H 64
#define OLED_FB_SIZE (OLED_W * OLED_H / 8)

void ssd1306_init(void);
void ssd1306_show(const uint8_t *fb); // fb: 1024 bytes, page layout
