#include "button.h"
#include "esp_log.h"

static const char *TAG = "BUTTON";

static void IRAM_ATTR button_isr_handler(void *args)
{
    QueueHandle_t button_queue = (QueueHandle_t)args;
    uint32_t button_event = 1;
    BaseType_t higher_priority_task_woken = pdFALSE;

    xQueueSendFromISR(button_queue, &button_event, &higher_priority_task_woken);

    if (higher_priority_task_woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

esp_err_t button_init(gpio_num_t button_gpio, QueueHandle_t button_queue)
{
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << button_gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };

    esp_err_t ret = gpio_config(&button_config);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = gpio_install_isr_service(0);

    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    ret = gpio_isr_handler_add(button_gpio, button_isr_handler, button_queue);
    if (ret != ESP_OK) {
        return ret;
    }

    ESP_LOGI(TAG, "Boton configurado en GPIO %d", button_gpio);

    return ESP_OK;
}