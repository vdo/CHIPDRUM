// =============================================================================
// Buttons, LEDs and ADC helpers
// =============================================================================
#pragma once
#include <stdbool.h>
#include <stdint.h>

void hardware_init(void);

bool button_read(int idx); // raw level, idx 0..5 (UP DOWN LEFT RIGHT E1 E2)

void led_set(int idx, bool on); // idx 0..6
void leds_all(bool on);
// LEDs 3-7 show the selected param slot (0 = MODE -> none lit)
void led_show_param_slot(int slot);
void led_startup_animation(void);

// Raw 12-bit ADC reads
uint16_t adc_read_cv1(void);
uint16_t adc_read_slider(void);
