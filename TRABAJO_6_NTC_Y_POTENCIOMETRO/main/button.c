#include "button.h"    // Declaración de button_init().
#include "esp_log.h"   // Permite logs.
#include "esp_attr.h"  // Permite IRAM_ATTR para la ISR.

static const char *TAG = "BUTTON"; // Etiqueta para logs.

// ISR del botón: se ejecuta cuando se detecta flanco de bajada.
static void IRAM_ATTR button_isr_handler(void *args)
{
    QueueHandle_t button_queue = (QueueHandle_t)args;       // Cola recibida.
    uint32_t button_event = 1;                              // Evento de botón.
    BaseType_t higher_priority_task_woken = pdFALSE;        // Bandera de FreeRTOS.

    xQueueSendFromISR(button_queue, &button_event, &higher_priority_task_woken); // Envía evento.

    if (higher_priority_task_woken == pdTRUE) {             // Si despertó una tarea...
        portYIELD_FROM_ISR();                               // Cambia de contexto si hace falta.
    }
}

// Configura el botón con pull-up interno e interrupción.
esp_err_t button_init(gpio_num_t button_gpio, QueueHandle_t button_queue)
{
    gpio_config_t button_config = {                         // Configuración del GPIO.
        .pin_bit_mask = (1ULL << button_gpio),              // Pin del botón.
        .mode = GPIO_MODE_INPUT,                            // Entrada.
        .pull_up_en = GPIO_PULLUP_ENABLE,                   // Pull-up interno.
        .pull_down_en = GPIO_PULLDOWN_DISABLE,              // Sin pull-down.
        .intr_type = GPIO_INTR_NEGEDGE                      // Interrumpe al presionar.
    };

    esp_err_t ret = gpio_config(&button_config);            // Aplica configuración.
    if (ret != ESP_OK) return ret;

    ret = gpio_install_isr_service(0);                      // Instala servicio ISR.
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) return ret; // Permite si ya estaba instalado.

    ret = gpio_isr_handler_add(button_gpio, button_isr_handler, button_queue); // Asocia ISR.
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "Boton configurado en GPIO %d", button_gpio); // Confirma.
    return ESP_OK;                                             // Correcto.
}
