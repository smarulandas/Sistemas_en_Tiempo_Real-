#ifndef BUTTON_H
#define BUTTON_H

#include "esp_err.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

esp_err_t button_init(gpio_num_t button_gpio, QueueHandle_t button_queue);

#endif