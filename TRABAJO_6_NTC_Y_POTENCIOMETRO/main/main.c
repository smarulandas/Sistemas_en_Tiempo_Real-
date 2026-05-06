#include <stdio.h>                 // Entrada/salida estándar.
#include <stdlib.h>                // Permite usar calloc() y free().

#include "freertos/FreeRTOS.h"     // Base de FreeRTOS.
#include "freertos/task.h"         // Permite crear tareas.
#include "freertos/queue.h"        // Permite usar colas.

#include "esp_log.h"               // Permite imprimir logs.
#include "esp_err.h"               // Manejo de errores ESP-IDF.

#include "board_config.h"          // Pines y constantes del proyecto.
#include "led_rgb.h"               // Control de LED RGB.
#include "analog.h"                // Lectura de ADC, potenciómetro y NTC.
#include "button.h"                // Configuración del botón.

// Estados del LED RGB configurable.
typedef enum {
    STATE_CONFIG_RED = 0,          // Configura intensidad roja.
    STATE_CONFIG_BLUE,             // Configura intensidad azul.
    STATE_CONFIG_GREEN,            // Configura intensidad verde.
    STATE_SHOW_COLOR               // Muestra la mezcla guardada.
} color_state_t;

// Contexto general para evitar variables globales de lógica.
typedef struct {
    led_rgb_t led_config;          // LED RGB controlado por potenciómetro y botón.
    led_rgb_t led_temperature;     // LED RGB controlado por temperatura.
    analog_t analog;               // Manejador ADC.
    QueueHandle_t button_queue;    // Cola de eventos del botón.
    color_state_t current_state;   // Estado actual.
    uint8_t saved_red;             // Valor guardado rojo.
    uint8_t saved_green;           // Valor guardado verde.
    uint8_t saved_blue;            // Valor guardado azul.
} app_context_t;

static const char *TAG = "MAIN";   // Etiqueta de logs.

// Convierte estado a texto para mostrar en monitor.
static const char *state_to_string(color_state_t state)
{
    switch (state) {
        case STATE_CONFIG_RED: return "CONFIG RED";     // Texto estado rojo.
        case STATE_CONFIG_BLUE: return "CONFIG BLUE";   // Texto estado azul.
        case STATE_CONFIG_GREEN: return "CONFIG GREEN"; // Texto estado verde.
        case STATE_SHOW_COLOR: return "SHOW COLOR";     // Texto estado mezcla.
        default: return "UNKNOWN";                      // Estado no válido.
    }
}

// Guarda el valor del potenciómetro en el color correspondiente.
static void save_current_state_value(app_context_t *app, uint8_t duty)
{
    switch (app->current_state) {
        case STATE_CONFIG_RED: app->saved_red = duty; break;       // Guarda rojo.
        case STATE_CONFIG_BLUE: app->saved_blue = duty; break;     // Guarda azul.
        case STATE_CONFIG_GREEN: app->saved_green = duty; break;   // Guarda verde.
        case STATE_SHOW_COLOR: break;                              // No guarda en mezcla.
        default: break;                                            // Seguridad.
    }
}

// Cambia al siguiente estado de forma circular.
static void go_to_next_state(app_context_t *app)
{
    app->current_state = (app->current_state + 1) % 4; // 0->1->2->3->0.
    ESP_LOGI(TAG, "Nuevo estado: %s", state_to_string(app->current_state)); // Muestra estado.
}

// Actualiza el LED configurable según el estado actual.
static void update_config_led_output(app_context_t *app, uint8_t pot_duty)
{
    switch (app->current_state) {
        case STATE_CONFIG_RED:
            app->saved_red = pot_duty;                                // Rojo sigue pot.
            led_rgb_set_color(&app->led_config, app->saved_red, 0, 0); // Solo rojo.
            break;

        case STATE_CONFIG_BLUE:
            app->saved_blue = pot_duty;                                // Azul sigue pot.
            led_rgb_set_color(&app->led_config, 0, 0, app->saved_blue); // Solo azul.
            break;

        case STATE_CONFIG_GREEN:
            app->saved_green = pot_duty;                                // Verde sigue pot.
            led_rgb_set_color(&app->led_config, 0, app->saved_green, 0); // Solo verde.
            break;

        case STATE_SHOW_COLOR:
            led_rgb_set_color(&app->led_config, app->saved_red, app->saved_green, app->saved_blue); // Mezcla.
            break;

        default:
            led_rgb_off(&app->led_config);                              // Apaga si hay error.
            break;
    }
}

// Tarea que lee el NTC y controla el LED de temperatura.
static void temperature_task(void *args)
{
    app_context_t *app = (app_context_t *)args; // Recibe contexto.

    while (1) {
        float temperature_c = 0.0f;             // Variable de temperatura.
        esp_err_t ret = analog_read_ntc_temperature(&app->analog, &temperature_c); // Lee NTC.

        if (ret == ESP_OK) {
            if (temperature_c < TEMP_BLUE_LIMIT_C) {
                led_rgb_set_color(&app->led_temperature, 0, 0, 255); // Azul: T < 25.
            } else if (temperature_c <= TEMP_RED_LIMIT_C) {
                led_rgb_set_color(&app->led_temperature, 0, 255, 0); // Verde: 25 a 35.
            } else {
                led_rgb_set_color(&app->led_temperature, 255, 0, 0); // Rojo: T > 35.
            }

            ESP_LOGI(TAG, "Temperatura NTC: %.2f °C", temperature_c); // Imprime.
        } else {
            ESP_LOGE(TAG, "Error leyendo NTC: %s", esp_err_to_name(ret)); // Error.
        }

        vTaskDelay(pdMS_TO_TICKS(TEMP_TASK_PERIOD_MS)); // Espera 500 ms.
    }
}

// Tarea que lee el potenciómetro y procesa el botón.
static void color_config_task(void *args)
{
    app_context_t *app = (app_context_t *)args; // Recibe contexto.
    uint32_t button_event = 0;                  // Evento recibido.
    TickType_t last_button_tick = 0;            // Última pulsación válida.

    ESP_LOGI(TAG, "Estado inicial: %s", state_to_string(app->current_state)); // Estado inicial.

    while (1) {
        uint8_t pot_duty = 0;                   // Duty del potenciómetro.
        esp_err_t ret = analog_read_pot_duty(&app->analog, &pot_duty); // Lee pot.

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error leyendo potenciómetro: %s", esp_err_to_name(ret)); // Error.
            vTaskDelay(pdMS_TO_TICKS(CONFIG_TASK_PERIOD_MS)); // Espera.
            continue;                           // Repite ciclo.
        }

        while (xQueueReceive(app->button_queue, &button_event, 0) == pdTRUE) { // Revisa botón.
            TickType_t now = xTaskGetTickCount(); // Tiempo actual.

            if ((now - last_button_tick) >= pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS)) { // Antirrebote.
                save_current_state_value(app, pot_duty); // Guarda valor antes de cambiar.
                go_to_next_state(app);                   // Cambia estado.
                last_button_tick = now;                  // Actualiza tiempo.
            }
        }

        update_config_led_output(app, pot_duty); // Actualiza LED configurable.
        vTaskDelay(pdMS_TO_TICKS(CONFIG_TASK_PERIOD_MS)); // Espera 50 ms.
    }
}

// Configura estructuras y canales de los dos LED RGB.
static esp_err_t configure_leds(app_context_t *app)
{
    app->led_config = (led_rgb_t) {             // LED del punto 2.
        .pin_r = LED_CONFIG_R_GPIO,
        .pin_g = LED_CONFIG_G_GPIO,
        .pin_b = LED_CONFIG_B_GPIO,
        .channel_r = LEDC_CHANNEL_0,
        .channel_g = LEDC_CHANNEL_1,
        .channel_b = LEDC_CHANNEL_2,
        .common_anode = RGB_COMMON_ANODE
    };

    app->led_temperature = (led_rgb_t) {        // LED del punto 1.
        .pin_r = LED_TEMP_R_GPIO,
        .pin_g = LED_TEMP_G_GPIO,
        .pin_b = LED_TEMP_B_GPIO,
        .channel_r = LEDC_CHANNEL_3,
        .channel_g = LEDC_CHANNEL_4,
        .channel_b = LEDC_CHANNEL_5,
        .common_anode = RGB_COMMON_ANODE
    };

    esp_err_t ret = led_rgb_driver_init();      // Configura temporizador PWM.
    if (ret != ESP_OK) return ret;

    ret = led_rgb_init(&app->led_config);       // Inicializa LED configurable.
    if (ret != ESP_OK) return ret;

    ret = led_rgb_init(&app->led_temperature);  // Inicializa LED temperatura.
    if (ret != ESP_OK) return ret;

    return ESP_OK;                              // Correcto.
}

// Función principal del ESP-IDF.
void app_main(void)
{
    esp_err_t ret;                              // Variable de error.
    app_context_t *app = calloc(1, sizeof(app_context_t)); // Crea contexto.

    if (app == NULL) {                          // Si falla memoria...
        ESP_LOGE(TAG, "No se pudo reservar memoria para app_context_t");
        return;
    }

    app->current_state = STATE_CONFIG_RED;      // Estado inicial.
    app->saved_red = 0;                         // Rojo inicial.
    app->saved_green = 0;                       // Verde inicial.
    app->saved_blue = 0;                        // Azul inicial.

    app->button_queue = xQueueCreate(10, sizeof(uint32_t)); // Cola del botón.
    if (app->button_queue == NULL) {
        ESP_LOGE(TAG, "No se pudo crear la cola del botón");
        free(app);
        return;
    }

    ret = configure_leds(app);                  // Configura LEDs.
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando LEDs: %s", esp_err_to_name(ret));
        return;
    }

    ret = analog_init(&app->analog, POT_ADC_CHANNEL, NTC_ADC_CHANNEL); // Configura ADC.
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando ADC: %s", esp_err_to_name(ret));
        return;
    }

    ret = button_init(BUTTON_GPIO, app->button_queue); // Configura botón.
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando botón: %s", esp_err_to_name(ret));
        return;
    }

    xTaskCreate(temperature_task, "temperature_task", 4096, app, 2, NULL); // Tarea NTC prioridad 2.
    xTaskCreate(color_config_task, "color_config_task", 4096, app, 3, NULL); // Tarea pot/botón prioridad 3.
}
