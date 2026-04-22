#include "rgb_led.h"

#include <stddef.h>

static uint32_t rgb_led_max_duty(const rgb_led_t *led)
{
    return (1U << led->duty_resolution) - 1U;
}

static uint32_t rgb_led_percent_to_duty(const rgb_led_t *led, uint8_t percent)
{
    if (percent > 100U) {
        percent = 100U;
    }

    uint32_t max_duty = rgb_led_max_duty(led);
    uint32_t duty = (max_duty * percent) / 100U;

    if (led->active_low) {
        duty = max_duty - duty;
    }

    return duty;
}

static uint8_t *rgb_led_percent_ref(rgb_led_t *led, rgb_channel_t channel)
{
    switch (channel) {
        case RGB_CHANNEL_RED:
            return &led->red_percent;
        case RGB_CHANNEL_GREEN:
            return &led->green_percent;
        case RGB_CHANNEL_BLUE:
            return &led->blue_percent;
        default:
            return NULL;
    }
}

static ledc_channel_t rgb_led_get_ledc_channel(const rgb_led_t *led, rgb_channel_t channel)
{
    switch (channel) {
        case RGB_CHANNEL_RED:
            return led->channel_r;
        case RGB_CHANNEL_GREEN:
            return led->channel_g;
        case RGB_CHANNEL_BLUE:
            return led->channel_b;
        default:
            return LEDC_CHANNEL_0;
    }
}

static esp_err_t rgb_led_apply_one(const rgb_led_t *led, rgb_channel_t channel, uint8_t percent)
{
    ledc_channel_t ledc_channel = rgb_led_get_ledc_channel(led, channel);
    uint32_t duty = rgb_led_percent_to_duty(led, percent);

    esp_err_t err = ledc_set_duty(led->speed_mode, ledc_channel, duty);
    if (err != ESP_OK) {
        return err;
    }

    return ledc_update_duty(led->speed_mode, ledc_channel);
}

esp_err_t rgb_led_init(rgb_led_t *led)
{
    if (led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ledc_timer_config_t timer_config = {
        .speed_mode = led->speed_mode,
        .timer_num = led->timer,
        .duty_resolution = led->duty_resolution,
        .freq_hz = led->pwm_freq_hz,
        .clk_cfg = LEDC_AUTO_CLK
    };

    esp_err_t err = ledc_timer_config(&timer_config);
    if (err != ESP_OK) {
        return err;
    }

    ledc_channel_config_t ch_r = {
        .gpio_num = led->pin_r,
        .speed_mode = led->speed_mode,
        .channel = led->channel_r,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = led->timer,
        .duty = 0,
        .hpoint = 0
    };

    ledc_channel_config_t ch_g = {
        .gpio_num = led->pin_g,
        .speed_mode = led->speed_mode,
        .channel = led->channel_g,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = led->timer,
        .duty = 0,
        .hpoint = 0
    };

    ledc_channel_config_t ch_b = {
        .gpio_num = led->pin_b,
        .speed_mode = led->speed_mode,
        .channel = led->channel_b,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = led->timer,
        .duty = 0,
        .hpoint = 0
    };

    err = ledc_channel_config(&ch_r);
    if (err != ESP_OK) {
        return err;
    }

    err = ledc_channel_config(&ch_g);
    if (err != ESP_OK) {
        return err;
    }

    err = ledc_channel_config(&ch_b);
    if (err != ESP_OK) {
        return err;
    }

    return rgb_led_apply(led);
}

esp_err_t rgb_led_apply(const rgb_led_t *led)
{
    if (led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = rgb_led_apply_one(led, RGB_CHANNEL_RED, led->red_percent);
    if (err != ESP_OK) {
        return err;
    }

    err = rgb_led_apply_one(led, RGB_CHANNEL_GREEN, led->green_percent);
    if (err != ESP_OK) {
        return err;
    }

    return rgb_led_apply_one(led, RGB_CHANNEL_BLUE, led->blue_percent);
}

esp_err_t rgb_led_set_percent(rgb_led_t *led, rgb_channel_t channel, uint8_t percent)
{
    if (led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t *value = rgb_led_percent_ref(led, channel);
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (percent > 100U) {
        percent = 100U;
    }

    *value = percent;
    return rgb_led_apply(led);
}

esp_err_t rgb_led_step_up(rgb_led_t *led, rgb_channel_t channel, uint8_t step_percent)
{
    if (led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t *value = rgb_led_percent_ref(led, channel);
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (*value >= 100U) {
        *value = 0U;
    } else {
        *value += step_percent;

        if (*value > 100U) {
            *value = 100U;
        }
    }

    return rgb_led_apply(led);
}

esp_err_t rgb_led_set_all(rgb_led_t *led, uint8_t red, uint8_t green, uint8_t blue)
{
    if (led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (red > 100U) {
        red = 100U;
    }

    if (green > 100U) {
        green = 100U;
    }

    if (blue > 100U) {
        blue = 100U;
    }

    led->red_percent = red;
    led->green_percent = green;
    led->blue_percent = blue;

    return rgb_led_apply(led);
}

uint8_t rgb_led_get_percent(const rgb_led_t *led, rgb_channel_t channel)
{
    if (led == NULL) {
        return 0U;
    }

    switch (channel) {
        case RGB_CHANNEL_RED:
            return led->red_percent;
        case RGB_CHANNEL_GREEN:
            return led->green_percent;
        case RGB_CHANNEL_BLUE:
            return led->blue_percent;
        default:
            return 0U;
    }
}