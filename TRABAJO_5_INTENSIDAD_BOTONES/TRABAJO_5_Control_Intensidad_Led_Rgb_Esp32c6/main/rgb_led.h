#ifndef RGB_LED_H
#define RGB_LED_H

// Librería para usar el tipo bool (true / false)
#include <stdbool.h>

// Librería para usar tipos enteros de tamaño fijo como uint8_t y uint32_t
#include <stdint.h>

// Librería de GPIO de ESP-IDF
#include "driver/gpio.h"

// Librería del periférico LEDC, que usaremos para generar PWM
#include "driver/ledc.h"

// Librería para manejar códigos de error tipo esp_err_t
#include "esp_err.h"

// Enumeración para identificar cada canal de color del LED RGB
typedef enum {
    RGB_CHANNEL_RED = 0,    // Canal rojo
    RGB_CHANNEL_GREEN,      // Canal verde
    RGB_CHANNEL_BLUE        // Canal azul
} rgb_channel_t;

// Estructura principal de la librería.
// Aquí se guarda toda la configuración del LED RGB:
// - pines
// - canales PWM
// - timer PWM
// - frecuencia
// - si el LED es activo en bajo o no
// - porcentaje actual de cada color
typedef struct {
    // Pines físicos del ESP32-C6 conectados al LED RGB
    gpio_num_t pin_r;   // Pin del color rojo
    gpio_num_t pin_g;   // Pin del color verde
    gpio_num_t pin_b;   // Pin del color azul

    // Canales LEDC usados para generar PWM en cada color
    ledc_channel_t channel_r;   // Canal PWM del rojo
    ledc_channel_t channel_g;   // Canal PWM del verde
    ledc_channel_t channel_b;   // Canal PWM del azul

    // Configuración general del temporizador PWM
    ledc_timer_t timer;                 // Timer LEDC usado
    ledc_mode_t speed_mode;             // Modo LEDC
    ledc_timer_bit_t duty_resolution;   // Resolución del duty cycle
    uint32_t pwm_freq_hz;               // Frecuencia PWM en Hz

    // Indica si el LED trabaja invertido
    // false = cátodo común
    // true  = ánodo común
    bool active_low;

    // Intensidad actual de cada color en porcentaje (0 a 100)
    uint8_t red_percent;
    uint8_t green_percent;
    uint8_t blue_percent;
} rgb_led_t;

// Inicializa el PWM del LED RGB con la configuración guardada en la estructura
esp_err_t rgb_led_init(rgb_led_t *led);

// Aplica al hardware los porcentajes actuales almacenados en la estructura
esp_err_t rgb_led_apply(const rgb_led_t *led);

// Fija directamente el porcentaje de un color específico
esp_err_t rgb_led_set_percent(rgb_led_t *led, rgb_channel_t channel, uint8_t percent);

// Incrementa un color en cierto porcentaje.
// En nuestra versión, si ya estaba en 100%, vuelve a 0%.
esp_err_t rgb_led_step_up(rgb_led_t *led, rgb_channel_t channel, uint8_t step_percent);

// Fija directamente los tres colores al mismo tiempo
esp_err_t rgb_led_set_all(rgb_led_t *led, uint8_t red, uint8_t green, uint8_t blue);

// Devuelve el porcentaje actual del color solicitado
uint8_t rgb_led_get_percent(const rgb_led_t *led, rgb_channel_t channel);

#endif