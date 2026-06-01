#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"

#include "analog.h"
#include "app_commands.h"
#include "board_config.h"
#include "button_unit.h"
#include "rgb_led.h"
#include "temp_control.h"

/*
 * ============================================================
 * CONTEXTO GENERAL DE LA APLICACIÓN
 * ============================================================
 *
 * Esta estructura guarda todos los módulos del sistema.
 *
 * No es una variable global.
 * Solo define un tipo de dato.
 */
typedef struct {
    analog_t analog;
    temp_control_t temp_control;
    rgb_led_t rgb1;
    rgb_led_t rgb2;
    button_unit_t unit_button;
} app_context_t;

/*
 * ============================================================
 * TAREA DE COMANDOS
 * ============================================================
 *
 * Lee comandos desde el monitor serial.
 * No usa variables globales.
 */
static void command_task(void *arg)
{
    app_context_t *app = (app_context_t *)arg;

    char command_line[96] = {0};     // Guarda el comando escrito por UART
    size_t command_index = 0;        // Indica la posición actual dentro del texto

    app_commands_print_help();

    printf("\nEscriba un comando y presione ENTER:\n> ");

    while (true) {
        int received_char = getchar();

        if (received_char != EOF) {
            /*
             * Si el usuario presiona ENTER, se procesa el comando completo.
             */
            if (received_char == '\n' || received_char == '\r') {
                printf("\n");

                command_line[command_index] = '\0';

                if (command_index > 0) {
                    app_commands_handle_line(
                        command_line,
                        &app->temp_control,
                        &app->rgb1,
                        &app->rgb2,
                        &app->analog
                    );
                }

                /*
                 * Limpia el buffer para recibir un nuevo comando.
                 */
                command_index = 0;
                memset(command_line, 0, sizeof(command_line));

                printf("> ");
            }

            /*
             * Permite borrar con Backspace.
             */
            else if (received_char == '\b' || received_char == 127) {
                if (command_index > 0) {
                    command_index--;

                    command_line[command_index] = '\0';

                    /*
                     * Borra visualmente el caracter en el monitor.
                     */
                    printf("\b \b");
                }
            }

            /*
             * Guarda caracteres normales.
             */
            else {
                if (command_index < sizeof(command_line) - 1) {
                    command_line[command_index] = (char)received_char;
                    command_index++;

                    /*
                     * Esto muestra en pantalla lo que vas escribiendo.
                     */
                    putchar(received_char);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*
 * ============================================================
 * TAREA PRINCIPAL DE CONTROL
 * ============================================================
 *
 * Esta tarea:
 * - Lee temperatura.
 * - Lee umbral del potenciómetro.
 * - Cambia unidad con el botón.
 * - Imprime temperatura cada X segundos.
 * - Controla RGB1 por rangos de temperatura.
 * - Controla RGB2 rojo por umbral.
 *
 * No usa variables globales.
 */
static void control_task(void *arg)
{
    app_context_t *app = (app_context_t *)arg;

    TickType_t last_print_tick = xTaskGetTickCount();

    while (true) {
        float temperature_c = 0.0f;
        uint8_t threshold_c = 0;

        /*
         * El botón cambia la unidad:
         * °C -> °F -> K -> °C
         */
        if (button_unit_pressed_event(&app->unit_button)) {
            temp_control_next_unit(&app->temp_control);

            printf(
                "Unidad cambiada con boton: %s\n",
                temp_control_unit_symbol(&app->temp_control)
            );
        }

        /*
         * Lee temperatura del NTC y umbral del potenciómetro.
         */
        if (analog_read_ntc_temperature(&app->analog, &temperature_c) == ESP_OK &&
            analog_read_pot_threshold_percent(&app->analog, &threshold_c) == ESP_OK) {

            /*
             * RGB1:
             * Muestra color según el rango de temperatura.
             */
            rgb_led_update_by_temperature_c(
                &app->rgb1,
                temperature_c
            );

            /*
             * RGB2:
             * Solo usa el rojo para el punto 4.
             *
             * Si temperatura >= umbral, prende rojo.
             * Si no, apaga el RGB2.
             */
            bool threshold_active =
                temperature_c >= (float)threshold_c;

            if (threshold_active) {
                rgb_led_set(
                    &app->rgb2,
                    app->rgb2.red.intensity,
                    0,
                    0
                );
            } else {
                rgb_led_off(&app->rgb2);
            }

            /*
             * Controla cada cuántos segundos se imprime.
             */
            TickType_t now_tick = xTaskGetTickCount();

            TickType_t elapsed_tick =
                now_tick - last_print_tick;

            TickType_t period_tick =
                pdMS_TO_TICKS(app->temp_control.print_period_s * 1000U);

            if (elapsed_tick >= period_tick) {
                float temperature_print =
                    temp_control_convert_from_c(
                        &app->temp_control,
                        temperature_c
                    );

                printf(
                    "Temperatura: %.2f %s | Umbral pot: %u °C | RGB2 rojo: %s\n",
                    temperature_print,
                    temp_control_unit_symbol(&app->temp_control),
                    threshold_c,
                    threshold_active ? "ON" : "OFF"
                );

                last_print_tick = now_tick;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/*
 * ============================================================
 * FUNCIÓN PRINCIPAL
 * ============================================================
 */
void app_main(void)
{
    /*
     * Se crea el contexto en memoria dinámica.
     *
     * Esto evita variables globales y mantiene los datos vivos
     * mientras las tareas se ejecutan.
     */
    app_context_t *app = calloc(1, sizeof(app_context_t));

    if (app == NULL) {
        printf("Error: no se pudo reservar memoria para app_context_t\n");
        return;
    }

    /*
     * Inicializa control de unidad y periodo.
     */
    temp_control_init(&app->temp_control);

    /*
     * Inicializa ADC para potenciómetro y NTC.
     */
    ESP_ERROR_CHECK(analog_init(
        &app->analog,
        POT_ADC_CHANNEL,
        NTC_ADC_CHANNEL
    ));

    /*
     * Inicializa RGB1 para mostrar temperatura.
     */
    ESP_ERROR_CHECK(rgb_led_init(
        &app->rgb1,
        RGB1_RED_GPIO,
        RGB1_GREEN_GPIO,
        RGB1_BLUE_GPIO,
        RGB1_RED_LEDC_CHANNEL,
        RGB1_GREEN_LEDC_CHANNEL,
        RGB1_BLUE_LEDC_CHANNEL
    ));

    /*
     * Inicializa RGB2 para el umbral del potenciómetro.
     */
    ESP_ERROR_CHECK(rgb_led_init(
        &app->rgb2,
        RGB2_RED_GPIO,
        RGB2_GREEN_GPIO,
        RGB2_BLUE_GPIO,
        RGB2_RED_LEDC_CHANNEL,
        RGB2_GREEN_LEDC_CHANNEL,
        RGB2_BLUE_LEDC_CHANNEL
    ));

    /*
     * El RGB2 solo usará rojo.
     * Se configura su intensidad inicial.
     */
    rgb_led_set_intensity(
        &app->rgb2,
        RGB_COLOR_RED,
        RGB2_RED_INTENSITY
    );

    /*
     * Inicializa botón para cambiar unidad.
     */
    ESP_ERROR_CHECK(button_unit_init(
        &app->unit_button,
        BUTTON_UNIT_GPIO
    ));

    /*
     * Crea tarea de control.
     * Se pasa app para evitar variables globales.
     */
    xTaskCreate(
        control_task,
        "control_task",
        4096,
        app,
        5,
        NULL
    );

    /*
     * Crea tarea de comandos.
     * También recibe app como argumento.
     */
    xTaskCreate(
        command_task,
        "command_task",
        4096,
        app,
        5,
        NULL
    );
}