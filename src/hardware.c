#include "hardware.h"

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/time.h"

#include "config.h"

static const uint8_t BTN_PINS[NUM_BUTTONS] = {PIN_BTN_UP,     PIN_BTN_DOWN,
                                              PIN_BTN_LEFT,   PIN_BTN_RIGHT,
                                              PIN_BTN_EXTRA1, PIN_BTN_EXTRA2};
static const uint8_t LED_PINS[NUM_LEDS] = {PIN_LED_1, PIN_LED_2, PIN_LED_3,
                                           PIN_LED_4, PIN_LED_5, PIN_LED_6,
                                           PIN_LED_7};

void hardware_init(void) {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        gpio_init(BTN_PINS[i]);
        gpio_set_dir(BTN_PINS[i], GPIO_IN);
        gpio_pull_down(BTN_PINS[i]);
    }
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_init(LED_PINS[i]);
        gpio_set_dir(LED_PINS[i], GPIO_OUT);
        gpio_put(LED_PINS[i], 0);
    }
    adc_init();
    adc_gpio_init(PIN_ADC_CV1);
    adc_gpio_init(PIN_ADC_SLIDER);
}

bool button_read(int idx) { return gpio_get(BTN_PINS[idx]); }

void led_set(int idx, bool on) { gpio_put(LED_PINS[idx], on); }

void leds_all(bool on) {
    for (int i = 0; i < NUM_LEDS; i++) gpio_put(LED_PINS[i], on);
}

void led_show_param_slot(int slot) {
    // LED1 (ext clock) and LED2 (clock pulse) are managed elsewhere
    for (int i = 2; i < NUM_LEDS; i++)
        gpio_put(LED_PINS[i], slot > 0 && (i - 2) == (slot - 1) % 5);
}

void led_startup_animation(void) {
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_put(LED_PINS[i], 1);
        sleep_ms(40);
    }
    sleep_ms(100);
    for (int i = NUM_LEDS - 1; i >= 0; i--) {
        gpio_put(LED_PINS[i], 0);
        sleep_ms(40);
    }
}

uint16_t adc_read_cv1(void) {
    adc_select_input(ADC_CV1);
    return adc_read();
}

uint16_t adc_read_slider(void) {
    adc_select_input(ADC_SLIDER);
    return adc_read();
}
