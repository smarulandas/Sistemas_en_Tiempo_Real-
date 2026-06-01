#ifndef BUTTON_UNIT_H
#define BUTTON_UNIT_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/*
 * ============================================================
 * ESTRUCTURA DEL BOTÓN
 * ============================================================
 *
 * Guarda el estado del botón para detectar una pulsación real.
 */
typedef struct {
    int gpio;
    int stable_level;
    int last_level;
    int64_t last_change_us;
} button_unit_t;

/*
 * Inicializa el botón.
 */
esp_err_t button_unit_init(
    button_unit_t *button,
    int gpio
);

/*
 * Devuelve true una sola vez cuando el botón se presiona.
 */
bool button_unit_pressed_event(
    button_unit_t *button
);

#endif