#include "button.h" // Incluye el archivo de cabecera para las funciones del botón
#include "esp_log.h" // Incluye el archivo de cabecera para el registro de logs
#include "esp_attr.h" // Incluye el archivo de cabecera para atributos de ESP

static const char *TAG = "BUTTON"; // Etiqueta para los logs del módulo botón

/*
 * ISR del botón.
 *
 * La interrupción no cambia el estado directamente.
 * Solo envía un evento a una cola.
 */
static void IRAM_ATTR button_isr_handler(void *args) // Función ISR del botón, atributo IRAM_ATTR para ejecución en RAM interna
{
    QueueHandle_t button_queue = (QueueHandle_t)args; // Convierte el argumento a puntero de cola
    uint32_t button_event = 1; // Define el valor del evento del botón
    BaseType_t higher_priority_task_woken = pdFALSE; // Inicializa la bandera para tareas de mayor prioridad despertadas

    xQueueSendFromISR(button_queue, &button_event, &higher_priority_task_woken); // Envía el evento a la cola desde la ISR

    if (higher_priority_task_woken == pdTRUE) { // Verifica si una tarea de mayor prioridad fue despertada
        portYIELD_FROM_ISR(); // Fuerza el cambio de contexto a la tarea de mayor prioridad
    }
}

/*
 * Configura el botón con pull-up interno.
 *
 * Conexión:
 * GPIO ---- botón ---- GND
 */
esp_err_t button_init(gpio_num_t button_gpio, QueueHandle_t button_queue) // Función para inicializar el botón con GPIO y cola especificados
{
    gpio_config_t button_config = { // Estructura de configuración para el GPIO del botón
        .pin_bit_mask = (1ULL << button_gpio), // Máscara de bits para el pin GPIO especificado
        .mode = GPIO_MODE_INPUT, // Configura el modo como entrada
        .pull_up_en = GPIO_PULLUP_ENABLE, // Habilita la resistencia pull-up interna
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Deshabilita la resistencia pull-down
        .intr_type = GPIO_INTR_NEGEDGE // Configura interrupción en flanco descendente
    };

    esp_err_t ret = gpio_config(&button_config); // Aplica la configuración del GPIO
    if (ret != ESP_OK) { // Verifica si la configuración falló
        return ret; // Retorna el error
    }

    ret = gpio_install_isr_service(0); // Instala el servicio de ISR con flags por defecto

    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) { // Verifica si la instalación falló (excepto si ya está instalado)
        return ret; // Retorna el error
    }

    ret = gpio_isr_handler_add(button_gpio, button_isr_handler, button_queue); // Agrega el manejador ISR para el GPIO
    if (ret != ESP_OK) { // Verifica si la adición del manejador falló
        return ret; // Retorna el error
    }

    ESP_LOGI(TAG, "Boton configurado en GPIO %d", button_gpio); // Registra un mensaje de información sobre la configuración

    return ESP_OK; // Retorna éxito
}