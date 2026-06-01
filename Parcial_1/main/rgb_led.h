#ifndef RGB_LED_H
#define RGB_LED_H

#include <stdint.h>

#include "esp_err.h"
#include "driver/ledc.h"

/*
 * ============================================================
 * COLORES DISPONIBLES
 * ============================================================
 */
typedef enum {
    RGB_COLOR_RED = 0,
    RGB_COLOR_GREEN,
    RGB_COLOR_BLUE
} rgb_color_t;

/*
 * Zona de temperatura de cada color.
 */
typedef struct {
    float min_c;
    float max_c;
    uint8_t intensity;
} rgb_zone_t;

/*
 * Estructura de un LED RGB.
 *
 * Sirve tanto para RGB1 como para RGB2.
 * No usa variables globales.
 */
typedef struct {
    int gpio_r;
    int gpio_g;
    int gpio_b;

    ledc_channel_t ch_r;
    ledc_channel_t ch_g;
    ledc_channel_t ch_b;

    rgb_zone_t red;
    rgb_zone_t green;
    rgb_zone_t blue;
} rgb_led_t;

/* Inicializa un RGB con GPIO y canales PWM específicos. */
esp_err_t rgb_led_init(
    rgb_led_t *rgb,
    int gpio_r,
    int gpio_g,
    int gpio_b,
    ledc_channel_t ch_r,
    ledc_channel_t ch_g,
    ledc_channel_t ch_b
);

/* Escribe directamente las intensidades R, G y B. */
esp_err_t rgb_led_set(
    rgb_led_t *rgb,
    uint8_t red,
    uint8_t green,
    uint8_t blue
);

/* Apaga completamente el RGB. */
esp_err_t rgb_led_off(
    rgb_led_t *rgb
);

/* Cambia el rango de temperatura de un color. */
void rgb_led_set_zone(
    rgb_led_t *rgb,
    rgb_color_t color,
    float min_c,
    float max_c
);

/* Cambia la intensidad de un color. */
void rgb_led_set_intensity(
    rgb_led_t *rgb,
    rgb_color_t color,
    uint8_t intensity
);

/* Actualiza RGB1 según la temperatura en °C. */
esp_err_t rgb_led_update_by_temperature_c(
    rgb_led_t *rgb,
    float temperature_c
);

/* Convierte texto red, green o blue a color. */
int rgb_led_parse_color(
    const char *text,
    rgb_color_t *color
);

#endif