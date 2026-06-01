#include "app_commands.h"

#include <stdio.h>
#include <string.h>

#include <stdint.h>

/*
 * Convierte una intensidad de porcentaje a PWM.
 *
 * El usuario escribe valores de 0 a 100.
 * Internamente el LED trabaja con PWM de 0 a 255.
 */
static uint8_t intensity_percent_to_pwm(unsigned int intensity_percent)
{
    if (intensity_percent > 100U) {
        intensity_percent = 100U;
    }

    return (uint8_t)((intensity_percent * 255U) / 100U);
}

/*
 * ============================================================
 * COMANDOS POR SERIAL
 * ============================================================
 *
 * Comandos disponibles:
 *
 * help
 * periodo 5
 * unidad C
 * unidad F
 * unidad K
 * rgb1_rango red 20 30
 * rgb1_rango green 30 40
 * rgb1_rango blue 40 60
 * rgb1_int red 255
 * rgb1_int green 120
 * rgb1_int blue 80
 * rgb2_int red 255
 * umbral
 */

void app_commands_print_help(void)
{
    printf("\nComandos disponibles:\n");
    printf("  help                         -> muestra esta ayuda\n");
    printf("  periodo <segundos>           -> cambia cada cuanto se imprime temperatura\n");
    printf("  unidad <C|F|K>               -> cambia la unidad de impresion\n");
    printf("  rgb1_rango <color> <a> <b>   -> cambia rango de RGB1 en °C\n");
    printf("  rgb1_int <color> <0-100>     -> cambia intensidad de RGB1 en porcentaje\n");
    printf("  rgb2_int red <0-100>         -> cambia intensidad roja de RGB2 en porcentaje\n");
    printf("  umbral                       -> lee umbral del potenciometro 0-100 °C\n\n");
}

void app_commands_handle_line(
    const char *line,
    temp_control_t *temp_control,
    rgb_led_t *rgb1,
    rgb_led_t *rgb2,
    analog_t *analog
)
{
    char command[24] = {0};

    if (sscanf(line, "%23s", command) != 1) {
        return;
    }

    if (strcmp(command, "help") == 0) {
        app_commands_print_help();
        return;
    }

    if (strcmp(command, "periodo") == 0) {
        unsigned int period_s = 0;

        if (sscanf(line, "%*s %u", &period_s) == 1) {
            temp_control_set_period_s(temp_control, period_s);

            printf(
                "Periodo de impresion = %lu s\n",
                (unsigned long)temp_control->print_period_s
            );
        } else {
            printf("Uso: periodo <segundos>\n");
        }

        return;
    }

    if (strcmp(command, "unidad") == 0) {
        char unit_text[8] = {0};
        temp_unit_t unit;

        if (sscanf(line, "%*s %7s", unit_text) == 1 &&
            temp_control_parse_unit(unit_text, &unit)) {
            temp_control_set_unit(temp_control, unit);

            printf(
                "Unidad actual = %s\n",
                temp_control_unit_symbol(temp_control)
            );
        } else {
            printf("Uso: unidad <C|F|K>\n");
        }

        return;
    }

    if (strcmp(command, "rgb1_rango") == 0) {
        char color_text[12] = {0};
        float min_c = 0.0f;
        float max_c = 0.0f;
        rgb_color_t color;

        if (sscanf(line, "%*s %11s %f %f", color_text, &min_c, &max_c) == 3 &&
            rgb_led_parse_color(color_text, &color)) {
            rgb_led_set_zone(rgb1, color, min_c, max_c);

            printf(
                "RGB1 rango %s = %.2f a %.2f °C\n",
                color_text,
                min_c,
                max_c
            );
        } else {
            printf("Uso: rgb1_rango <red|green|blue> <min_C> <max_C>\n");
        }

        return;
    }

    if (strcmp(command, "rgb1_int") == 0) {
    char color_text[12] = {0};              // Guarda el color escrito por UART
    unsigned int intensity_percent = 0;     // Intensidad escrita por el usuario de 0 a 100
    rgb_color_t color;                      // Color interpretado por el programa

    /*
     * Ejemplo de comando:
     *
     * rgb1_int red 80
     *
     * Significa:
     * rojo al 80% de intensidad.
     */
    if (sscanf(line, "%*s %11s %u", color_text, &intensity_percent) == 2 &&
        rgb_led_parse_color(color_text, &color) &&
        intensity_percent <= 100U) {

        /*
         * Se convierte de porcentaje 0-100 a PWM 0-255.
         */
        uint8_t pwm_intensity = intensity_percent_to_pwm(intensity_percent);

        rgb_led_set_intensity(
            rgb1,
            color,
            pwm_intensity
        );

        printf(
            "RGB1 intensidad %s = %u%%\n",
            color_text,
            intensity_percent
        );
    } else {
        printf("Uso: rgb1_int <red|green|blue> <0-100>\n");
    }

    return;
}

    if (strcmp(command, "rgb2_int") == 0) {
    char color_text[12] = {0};              // Guarda el color escrito por UART
    unsigned int intensity_percent = 0;     // Intensidad escrita por el usuario de 0 a 100
    rgb_color_t color;                      // Color interpretado por el programa

    /*
     * Ejemplo de comando:
     *
     * rgb2_int red 70
     *
     * Significa:
     * rojo del RGB2 al 70% de intensidad.
     */
    if (sscanf(line, "%*s %11s %u", color_text, &intensity_percent) == 2 &&
        rgb_led_parse_color(color_text, &color) &&
        color == RGB_COLOR_RED &&
        intensity_percent <= 100U) {

        /*
         * Se convierte de porcentaje 0-100 a PWM 0-255.
         */
        uint8_t pwm_intensity = intensity_percent_to_pwm(intensity_percent);

        rgb_led_set_intensity(
            rgb2,
            RGB_COLOR_RED,
            pwm_intensity
        );

        printf(
            "RGB2 intensidad roja = %u%%\n",
            intensity_percent
        );
    } else {
        printf("Uso: rgb2_int red <0-100>\n");
    }

    return;
}

    if (strcmp(command, "umbral") == 0) {
        uint8_t threshold = 0;

        if (analog_read_pot_threshold_percent(analog, &threshold) == ESP_OK) {
            printf("Umbral actual = %u °C\n", threshold);
        } else {
            printf("Error leyendo el potenciometro\n");
        }

        return;
    }

    printf("Comando no reconocido. Escribe: help\n");
}