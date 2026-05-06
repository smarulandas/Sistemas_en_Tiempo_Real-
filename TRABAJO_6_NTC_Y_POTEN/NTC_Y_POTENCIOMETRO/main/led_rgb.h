#ifndef LED_RGB_H
#define LED_RGB_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

typedef struct {
    gpio_num_t pin_r;
    gpio_num_t pin_g;
    gpio_num_t pin_b;

    ledc_channel_t channel_r;
    ledc_channel_t channel_g;
    ledc_channel_t channel_b;

    bool common_anode;
} led_rgb_t;

esp_err_t led_rgb_driver_init(void);

esp_err_t led_rgb_init(led_rgb_t *led);

esp_err_t led_rgb_set_color(led_rgb_t *led, uint8_t red, uint8_t green, uint8_t blue);

esp_err_t led_rgb_off(led_rgb_t *led);

#endif