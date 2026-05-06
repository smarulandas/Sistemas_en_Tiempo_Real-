#ifndef BUTTON_H
#define BUTTON_H

#include "esp_err.h"              // Tipo esp_err_t.
#include "driver/gpio.h"          // Configuración GPIO.
#include "freertos/FreeRTOS.h"    // Base FreeRTOS.
#include "freertos/queue.h"       // Colas FreeRTOS.

esp_err_t button_init(gpio_num_t button_gpio, QueueHandle_t button_queue); // Inicializa botón.

#endif
