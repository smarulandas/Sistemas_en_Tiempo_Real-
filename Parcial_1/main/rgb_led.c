#include "rgb_led.h"
#include "board_config.h"

#include <stdbool.h>
#include <strings.h>

#include "esp_log.h"

/*
 * No es variable global.
 * Es una macro usada para mensajes de log.
 */
#define TAG "RGB_LED"

#define RGB_TIMER              LEDC_TIMER_0
#define RGB_SPEED_MODE         LEDC_LOW_SPEED_MODE
#define RGB_DUTY_RESOLUTION    LEDC_TIMER_8_BIT
#define RGB_DUTY_MAX           255

/*
 * ============================================================
 * CONTROL DE LED RGB POR PWM
 * ============================================================
 *
 * Esta librería sirve para:
 * - RGB1: colores por temperatura.
 * - RGB2: rojo por umbral del potenciómetro.
 */

static uint32_t rgb_led_apply_polarity(
    uint8_t value
)
{
#if RGB_ACTIVE_HIGH
    return value;
#else
    return RGB_DUTY_MAX - value;
#endif
}

static esp_err_t rgb_led_config_channel(
    int gpio,
    ledc_channel_t channel
)
{
    ledc_channel_config_t channel_config = {
        .gpio_num = gpio,
        .speed_mode = RGB_SPEED_MODE,
        .channel = channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = RGB_TIMER,
        .duty = 0,
        .hpoint = 0
    };

    return ledc_channel_config(&channel_config);
}

esp_err_t rgb_led_init(
    rgb_led_t *rgb,
    int gpio_r,
    int gpio_g,
    int gpio_b,
    ledc_channel_t ch_r,
    ledc_channel_t ch_g,
    ledc_channel_t ch_b
)
{
    rgb->gpio_r = gpio_r;
    rgb->gpio_g = gpio_g;
    rgb->gpio_b = gpio_b;

    rgb->ch_r = ch_r;
    rgb->ch_g = ch_g;
    rgb->ch_b = ch_b;

    /*
     * Carga valores iniciales.
     *
     * Para RGB1 se usan como rangos reales.
     * Para RGB2 solo nos interesa red.intensity.
     */
    rgb->red.min_c = RGB1_RED_MIN_C;
    rgb->red.max_c = RGB1_RED_MAX_C;
    rgb->red.intensity = RGB1_RED_INTENSITY;

    rgb->green.min_c = RGB1_GREEN_MIN_C;
    rgb->green.max_c = RGB1_GREEN_MAX_C;
    rgb->green.intensity = RGB1_GREEN_INTENSITY;

    rgb->blue.min_c = RGB1_BLUE_MIN_C;
    rgb->blue.max_c = RGB1_BLUE_MAX_C;
    rgb->blue.intensity = RGB1_BLUE_INTENSITY;

    ledc_timer_config_t timer_config = {
        .speed_mode = RGB_SPEED_MODE,
        .duty_resolution = RGB_DUTY_RESOLUTION,
        .timer_num = RGB_TIMER,
        .freq_hz = RGB_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };

    esp_err_t ret = ledc_timer_config(&timer_config);

    if (ret != ESP_OK) {
        return ret;
    }

    ret = rgb_led_config_channel(gpio_r, rgb->ch_r);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = rgb_led_config_channel(gpio_g, rgb->ch_g);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = rgb_led_config_channel(gpio_b, rgb->ch_b);
    if (ret != ESP_OK) {
        return ret;
    }

    ESP_LOGI(TAG, "RGB inicializado");

    return rgb_led_off(rgb);
}

esp_err_t rgb_led_set(
    rgb_led_t *rgb,
    uint8_t red,
    uint8_t green,
    uint8_t blue
)
{
    esp_err_t ret = ESP_OK;

    ret |= ledc_set_duty(
        RGB_SPEED_MODE,
        rgb->ch_r,
        rgb_led_apply_polarity(red)
    );

    ret |= ledc_update_duty(
        RGB_SPEED_MODE,
        rgb->ch_r
    );

    ret |= ledc_set_duty(
        RGB_SPEED_MODE,
        rgb->ch_g,
        rgb_led_apply_polarity(green)
    );

    ret |= ledc_update_duty(
        RGB_SPEED_MODE,
        rgb->ch_g
    );

    ret |= ledc_set_duty(
        RGB_SPEED_MODE,
        rgb->ch_b,
        rgb_led_apply_polarity(blue)
    );

    ret |= ledc_update_duty(
        RGB_SPEED_MODE,
        rgb->ch_b
    );

    return ret;
}

esp_err_t rgb_led_off(
    rgb_led_t *rgb
)
{
    return rgb_led_set(rgb, 0, 0, 0);
}

static rgb_zone_t *rgb_led_get_zone(
    rgb_led_t *rgb,
    rgb_color_t color
)
{
    switch (color) {
        case RGB_COLOR_GREEN:
            return &rgb->green;

        case RGB_COLOR_BLUE:
            return &rgb->blue;

        case RGB_COLOR_RED:
        default:
            return &rgb->red;
    }
}

void rgb_led_set_zone(
    rgb_led_t *rgb,
    rgb_color_t color,
    float min_c,
    float max_c
)
{
    rgb_zone_t *zone = rgb_led_get_zone(rgb, color);

    zone->min_c = min_c;
    zone->max_c = max_c;
}

void rgb_led_set_intensity(
    rgb_led_t *rgb,
    rgb_color_t color,
    uint8_t intensity
)
{
    rgb_zone_t *zone = rgb_led_get_zone(rgb, color);

    zone->intensity = intensity;
}

static bool rgb_led_temperature_in_zone(
    rgb_zone_t *zone,
    float temperature_c
)
{
    return temperature_c >= zone->min_c &&
           temperature_c <= zone->max_c;
}

esp_err_t rgb_led_update_by_temperature_c(
    rgb_led_t *rgb,
    float temperature_c
)
{
    uint8_t red_value = 0;     // Intensidad roja que se aplicará
    uint8_t green_value = 0;   // Intensidad verde que se aplicará
    uint8_t blue_value = 0;    // Intensidad azul que se aplicará

    /*
     * Si la temperatura está dentro del rango rojo,
     * se activa el rojo con su intensidad configurada.
     */
    if (rgb_led_temperature_in_zone(&rgb->red, temperature_c)) {
        red_value = rgb->red.intensity;
    }

    /*
     * Si la temperatura está dentro del rango verde,
     * se activa el verde con su intensidad configurada.
     */
    if (rgb_led_temperature_in_zone(&rgb->green, temperature_c)) {
        green_value = rgb->green.intensity;
    }

    /*
     * Si la temperatura está dentro del rango azul,
     * se activa el azul con su intensidad configurada.
     */
    if (rgb_led_temperature_in_zone(&rgb->blue, temperature_c)) {
        blue_value = rgb->blue.intensity;
    }

    /*
     * Se mandan los tres colores al mismo tiempo.
     * Así, si varios rangos coinciden, se mezclan los colores.
     */
    return rgb_led_set(
        rgb,
        red_value,
        green_value,
        blue_value
    );
}

int rgb_led_parse_color(
    const char *text,
    rgb_color_t *color
)
{
    if (text == NULL || color == NULL) {
        return 0;
    }

    if (strcasecmp(text, "red") == 0 ||
        strcasecmp(text, "r") == 0) {
        *color = RGB_COLOR_RED;
        return 1;
    }

    if (strcasecmp(text, "green") == 0 ||
        strcasecmp(text, "g") == 0) {
        *color = RGB_COLOR_GREEN;
        return 1;
    }

    if (strcasecmp(text, "blue") == 0 ||
        strcasecmp(text, "b") == 0) {
        *color = RGB_COLOR_BLUE;
        return 1;
    }

    return 0;
}