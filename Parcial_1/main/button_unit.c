#include "button_unit.h"
#include "board_config.h"

#include "driver/gpio.h"
#include "esp_timer.h"

#define BUTTON_DEBOUNCE_US 40000

/*
 * ============================================================
 * BOTÓN PARA CAMBIAR UNIDAD
 * ============================================================
 *
 * Cada pulsación cambia:
 *
 * °C -> °F -> K -> °C
 *
 * No se usan variables globales.
 */
esp_err_t button_unit_init(
    button_unit_t *button,
    int gpio
)
{
    button->gpio = gpio;
    button->last_change_us = esp_timer_get_time();

    gpio_config_t config = {
        .pin_bit_mask = (1ULL << gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = BUTTON_ACTIVE_LEVEL == 0 ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = BUTTON_ACTIVE_LEVEL == 1 ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t ret = gpio_config(&config);

    if (ret != ESP_OK) {
        return ret;
    }

    button->stable_level = gpio_get_level(gpio);
    button->last_level = button->stable_level;

    return ESP_OK;
}

/*
 * ============================================================
 * DETECCIÓN DE PULSACIÓN
 * ============================================================
 *
 * Devuelve true solo una vez cuando el botón se presiona.
 */
bool button_unit_pressed_event(
    button_unit_t *button
)
{
    int current_level = gpio_get_level(button->gpio);
    int64_t now_us = esp_timer_get_time();

    if (current_level != button->last_level) {
        button->last_level = current_level;
        button->last_change_us = now_us;
    }

    if ((now_us - button->last_change_us) < BUTTON_DEBOUNCE_US) {
        return false;
    }

    if (current_level != button->stable_level) {
        button->stable_level = current_level;

        if (button->stable_level == BUTTON_ACTIVE_LEVEL) {
            return true;
        }
    }

    return false;
}