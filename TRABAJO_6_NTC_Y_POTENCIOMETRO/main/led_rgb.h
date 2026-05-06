#ifndef LED_RGB_H
#define LED_RGB_H

#include <stdint.h>          // Permite usar uint8_t.
#include <stdbool.h>         // Permite usar bool, true y false.
#include "esp_err.h"         // Permite usar esp_err_t.
#include "driver/gpio.h"     // Permite usar gpio_num_t.
#include "driver/ledc.h"     // Permite usar canales PWM LEDC.

// Estructura que representa un LED RGB.
typedef struct {
    gpio_num_t pin_r;              // GPIO conectado al color rojo.
    gpio_num_t pin_g;              // GPIO conectado al color verde.
    gpio_num_t pin_b;              // GPIO conectado al color azul.
    ledc_channel_t channel_r;      // Canal PWM usado por el color rojo.
    ledc_channel_t channel_g;      // Canal PWM usado por el color verde.
    ledc_channel_t channel_b;      // Canal PWM usado por el color azul.
    bool common_anode;             // true si el LED es ánodo común.
} led_rgb_t;

esp_err_t led_rgb_driver_init(void);  // Inicializa el temporizador PWM LEDC.
esp_err_t led_rgb_init(led_rgb_t *led);  // Configura los tres canales de un LED RGB.
esp_err_t led_rgb_set_color(led_rgb_t *led, uint8_t red, uint8_t green, uint8_t blue); // Asigna intensidad RGB.
esp_err_t led_rgb_off(led_rgb_t *led);   // Apaga el LED RGB.

#endif
