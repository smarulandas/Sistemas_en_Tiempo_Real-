#ifndef BUTTON_H  // Guardia de inclusión para prevenir múltiples inclusiones de este archivo
#define BUTTON_H  // Define la macro de guardia de inclusión

#include "esp_err.h"  // Incluir tipos de manejo de errores de ESP
#include "driver/gpio.h"  // Incluir controlador GPIO para control de pines
#include "freertos/FreeRTOS.h"  // Incluir definiciones base de FreeRTOS
#include "freertos/queue.h"  // Incluir definiciones de cola de FreeRTOS

esp_err_t button_init(gpio_num_t button_gpio, QueueHandle_t button_queue);  // Función para inicializar un botón en el GPIO especificado con una cola para eventos

#endif  // Fin de la guardia de inclusión