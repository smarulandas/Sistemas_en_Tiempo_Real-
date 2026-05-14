#ifndef LED_RGB_H  // Guarda de inclusión múltiple para evitar inclusiones repetidas del archivo
#define LED_RGB_H  // Define la guarda de inclusión

#include <stdint.h>  // Incluye tipos de datos enteros estándar
#include <stdbool.h>  // Incluye tipos booleanos estándar
#include "esp_err.h"  // Incluye definiciones de errores de ESP-IDF
#include "driver/gpio.h"  // Incluye funciones para control de GPIO
#include "driver/ledc.h"  // Incluye funciones para control de LEDC (PWM)

/*
 * Estructura para representar un LED RGB.
 */
typedef struct {  // Define una estructura para configurar un LED RGB
    gpio_num_t pin_r;  // Pin GPIO asignado al color rojo
    gpio_num_t pin_g;  // Pin GPIO asignado al color verde
    gpio_num_t pin_b;  // Pin GPIO asignado al color azul

    ledc_channel_t channel_r;  // Canal LEDC para el PWM del rojo
    ledc_channel_t channel_g;  // Canal LEDC para el PWM del verde
    ledc_channel_t channel_b;  // Canal LEDC para el PWM del azul

    bool common_anode;  // Indica si el LED es de ánodo común (true) o cátodo común (false)
} led_rgb_t;  // Nombre del tipo de estructura

esp_err_t led_rgb_driver_init(void);  // Función para inicializar el driver LEDC

esp_err_t led_rgb_init(led_rgb_t *led);  // Función para inicializar un LED RGB específico

esp_err_t led_rgb_set_color(led_rgb_t *led, uint8_t red, uint8_t green, uint8_t blue);  // Función para establecer el color del LED RGB

esp_err_t led_rgb_off(led_rgb_t *led);  // Función para apagar el LED RGB

#endif  // Fin de la guarda de inclusión múltiple