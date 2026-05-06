#include "led_rgb.h"
#include "esp_log.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RESOLUTION    LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY_HZ       4000

static const char *TAG = "LED_RGB";

/*
 * Convierte un valor de 0 a 255 en duty PWM.
 *
 * Para cátodo común:
 *      0   = apagado
 *      255 = máximo brillo
 *
 * Para ánodo común:
 *      0   = máximo brillo físico
 *      255 = apagado físico
 *
 * Por eso se invierte cuando common_anode = true.
 */
static uint32_t led_rgb_apply_polarity(const led_rgb_t *led, uint8_t value)
{
    if (led->common_anode) {
        return 255 - value;
    }

    return value;
}

/*
 * Configura el temporizador PWM LEDC.
 * Esto se hace una sola vez para todos los LEDs RGB.
 */
esp_err_t led_rgb_driver_init(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RESOLUTION,
        .freq_hz = LEDC_FREQUENCY_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };

    esp_err_t ret = ledc_timer_config(&timer_config);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Temporizador LEDC configurado correctamente");
    }

    return ret;
}

/*
 * Configura un canal PWM individual.
 */
static esp_err_t led_rgb_config_channel(gpio_num_t pin, ledc_channel_t channel)
{
    ledc_channel_config_t channel_config = {
        .gpio_num = pin,
        .speed_mode = LEDC_MODE,
        .channel = channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };

    return ledc_channel_config(&channel_config);
}

/*
 * Inicializa los tres canales PWM de un LED RGB.
 */
esp_err_t led_rgb_init(led_rgb_t *led)
{
    esp_err_t ret;

    ret = led_rgb_config_channel(led->pin_r, led->channel_r);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = led_rgb_config_channel(led->pin_g, led->channel_g);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = led_rgb_config_channel(led->pin_b, led->channel_b);
    if (ret != ESP_OK) {
        return ret;
    }

    return led_rgb_off(led);
}

/*
 * Cambia el color completo del LED RGB.
 */
esp_err_t led_rgb_set_color(led_rgb_t *led, uint8_t red, uint8_t green, uint8_t blue)
{
    esp_err_t ret;

    ret = ledc_set_duty(LEDC_MODE, led->channel_r, led_rgb_apply_polarity(led, red));
    if (ret != ESP_OK) {
        return ret;
    }

    ret = ledc_update_duty(LEDC_MODE, led->channel_r);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = ledc_set_duty(LEDC_MODE, led->channel_g, led_rgb_apply_polarity(led, green));
    if (ret != ESP_OK) {
        return ret;
    }

    ret = ledc_update_duty(LEDC_MODE, led->channel_g);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = ledc_set_duty(LEDC_MODE, led->channel_b, led_rgb_apply_polarity(led, blue));
    if (ret != ESP_OK) {
        return ret;
    }

    ret = ledc_update_duty(LEDC_MODE, led->channel_b);
    if (ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

/*
 * Apaga el LED RGB.
 */
esp_err_t led_rgb_off(led_rgb_t *led)
{
    return led_rgb_set_color(led, 0, 0, 0);
}