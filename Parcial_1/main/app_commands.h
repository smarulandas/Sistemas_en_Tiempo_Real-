#ifndef APP_COMMANDS_H
#define APP_COMMANDS_H

#include "analog.h"
#include "rgb_led.h"
#include "temp_control.h"

/*
 * ============================================================
 * COMANDOS DEL SISTEMA
 * ============================================================
 *
 * Esta librería procesa comandos recibidos por serial.
 *
 * No usa variables globales.
 */

/*
 * Procesa una línea escrita por el usuario.
 */
void app_commands_handle_line(
    const char *line,
    temp_control_t *temp_control,
    rgb_led_t *rgb1,
    rgb_led_t *rgb2,
    analog_t *analog
);

/*
 * Imprime la ayuda de comandos disponibles.
 */
void app_commands_print_help(void);

#endif