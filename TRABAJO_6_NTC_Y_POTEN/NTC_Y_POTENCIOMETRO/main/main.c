#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_err.h"

#include "board_config.h"
#include "led_rgb.h"
#include "analog.h"
#include "button.h"

/*
 * ============================================================
 * ESTADOS DEL PUNTO 2
 * ============================================================
 */
typedef enum {
    STATE_CONFIG_RED = 0,
    STATE_CONFIG_BLUE,
    STATE_CONFIG_GREEN,
    STATE_SHOW_COLOR
} color_state_t;

/*
 * ============================================================
 * CONTEXTO GENERAL DE LA APLICACIÓN
 * ============================================================
 *
 * Aquí se guardan todos los objetos necesarios.
 *
 * No usamos variables globales para guardar el estado del programa.
 * Este contexto se crea en app_main y se pasa a las tareas.
 */
typedef struct {
    led_rgb_t led_config;
    led_rgb_t led_temperature;

    analog_t analog;

    QueueHandle_t button_queue;

    color_state_t current_state;

    uint8_t saved_red;
    uint8_t saved_green;
    uint8_t saved_blue;
} app_context_t;

static const char *TAG = "MAIN";

/*
 * Devuelve el nombre del estado actual.
 * Sirve para imprimir mensajes claros en el monitor serial.
 */
static const char *state_to_string(color_state_t state)
{
    switch (state) {
        case STATE_CONFIG_RED:
            return "CONFIG RED";

        case STATE_CONFIG_BLUE:
            return "CONFIG BLUE";

        case STATE_CONFIG_GREEN:
            return "CONFIG GREEN";

        case STATE_SHOW_COLOR:
            return "SHOW COLOR";

        default:
            return "UNKNOWN";
    }
}

/*
 * Guarda el valor actual del potenciómetro según el estado.
 *
 * Nota:
 * En SHOW_COLOR no se guarda nada porque ahí solo se muestra
 * la mezcla final.
 */
static void save_current_state_value(app_context_t *app, uint8_t duty)
{
    switch (app->current_state) {
        case STATE_CONFIG_RED:
            app->saved_red = duty;
            break;

        case STATE_CONFIG_BLUE:
            app->saved_blue = duty;
            break;

        case STATE_CONFIG_GREEN:
            app->saved_green = duty;
            break;

        case STATE_SHOW_COLOR:
            break;

        default:
            break;
    }
}

/*
 * Cambia al siguiente estado.
 *
 * Orden solicitado:
 *
 *      CONFIG RED
 *      CONFIG BLUE
 *      CONFIG GREEN
 *      SHOW COLOR
 *      vuelve a CONFIG RED
 */
static void go_to_next_state(app_context_t *app)
{
    app->current_state = (app->current_state + 1) % 4;

    ESP_LOGI(TAG, "Nuevo estado: %s", state_to_string(app->current_state));
}

/*
 * Aplica la salida del LED RGB configurable según el estado actual.
 *
 * Punto importante del enunciado:
 * En los estados CONFIG RED, CONFIG BLUE y CONFIG GREEN,
 * solo debe estar encendido el color que se está configurando.
 *
 * En SHOW COLOR se muestra la mezcla de los tres valores guardados.
 */
static void update_config_led_output(app_context_t *app, uint8_t pot_duty)
{
    switch (app->current_state) {
        case STATE_CONFIG_RED:
            app->saved_red = pot_duty;
            led_rgb_set_color(&app->led_config, app->saved_red, 0, 0);
            break;

        case STATE_CONFIG_BLUE:
            app->saved_blue = pot_duty;
            led_rgb_set_color(&app->led_config, 0, 0, app->saved_blue);
            break;

        case STATE_CONFIG_GREEN:
            app->saved_green = pot_duty;
            led_rgb_set_color(&app->led_config, 0, app->saved_green, 0);
            break;

        case STATE_SHOW_COLOR:
            led_rgb_set_color(
                &app->led_config,
                app->saved_red,
                app->saved_green,
                app->saved_blue
            );
            break;

        default:
            led_rgb_off(&app->led_config);
            break;
    }
}

/*
 * ============================================================
 * TAREA DEL PUNTO 1
 * ============================================================
 *
 * Lee el NTC y cambia el color del primer LED RGB:
 *
 *      Azul  si T < 25 °C
 *      Verde si 25 °C <= T <= 35 °C
 *      Rojo  si T > 35 °C
 */
static void temperature_task(void *args)
{
    app_context_t *app = (app_context_t *)args;

    while (1) {
        float temperature_c = 0.0f;

        esp_err_t ret = analog_read_ntc_temperature(&app->analog, &temperature_c);

        if (ret == ESP_OK) {
            if (temperature_c < TEMP_BLUE_LIMIT_C) {
                led_rgb_set_color(&app->led_temperature, 0, 0, 255);
            }
            else if (temperature_c <= TEMP_RED_LIMIT_C) {
                led_rgb_set_color(&app->led_temperature, 0, 255, 0);
            }
            else {
                led_rgb_set_color(&app->led_temperature, 255, 0, 0);
            }

            ESP_LOGI(TAG, "Temperatura NTC: %.2f °C", temperature_c);
        }
        else {
            ESP_LOGE(TAG, "Error leyendo NTC: %s", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(TEMP_TASK_PERIOD_MS));
    }
}

/*
 * ============================================================
 * TAREA DEL PUNTO 2
 * ============================================================
 *
 * Lee el potenciómetro.
 * Controla la máquina de estados.
 * Recibe eventos del botón mediante una cola.
 */
static void color_config_task(void *args)
{
    app_context_t *app = (app_context_t *)args;

    uint32_t button_event = 0;
    TickType_t last_button_tick = 0;

    ESP_LOGI(TAG, "Estado inicial: %s", state_to_string(app->current_state));

    while (1) {
        uint8_t pot_duty = 0;

        esp_err_t ret = analog_read_pot_duty(&app->analog, &pot_duty);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error leyendo potenciómetro: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(CONFIG_TASK_PERIOD_MS));
            continue;
        }

        /*
         * Se revisa si el botón fue presionado.
         * La interrupción solo manda el evento.
         * El cambio de estado ocurre aquí.
         */
        while (xQueueReceive(app->button_queue, &button_event, 0) == pdTRUE) {
            TickType_t now = xTaskGetTickCount();

            if ((now - last_button_tick) >= pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS)) {
                save_current_state_value(app, pot_duty);
                go_to_next_state(app);
                last_button_tick = now;
            }
        }

        update_config_led_output(app, pot_duty);

        vTaskDelay(pdMS_TO_TICKS(CONFIG_TASK_PERIOD_MS));
    }
}

/*
 * Configura los dos LEDs RGB.
 */
static esp_err_t configure_leds(app_context_t *app)
{
    app->led_config = (led_rgb_t) {
        .pin_r = LED_CONFIG_R_GPIO,
        .pin_g = LED_CONFIG_G_GPIO,
        .pin_b = LED_CONFIG_B_GPIO,
        .channel_r = LEDC_CHANNEL_0,
        .channel_g = LEDC_CHANNEL_1,
        .channel_b = LEDC_CHANNEL_2,
        .common_anode = RGB_COMMON_ANODE
    };

    app->led_temperature = (led_rgb_t) {
        .pin_r = LED_TEMP_R_GPIO,
        .pin_g = LED_TEMP_G_GPIO,
        .pin_b = LED_TEMP_B_GPIO,
        .channel_r = LEDC_CHANNEL_3,
        .channel_g = LEDC_CHANNEL_4,
        .channel_b = LEDC_CHANNEL_5,
        .common_anode = RGB_COMMON_ANODE
    };

    esp_err_t ret = led_rgb_driver_init();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = led_rgb_init(&app->led_config);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = led_rgb_init(&app->led_temperature);
    if (ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

/*
 * Función principal de ESP-IDF.
 */
void app_main(void)
{
    esp_err_t ret;

    app_context_t *app = calloc(1, sizeof(app_context_t));

    if (app == NULL) {
        ESP_LOGE(TAG, "No se pudo reservar memoria para app_context_t");
        return;
    }

    app->current_state = STATE_CONFIG_RED;
    app->saved_red = 0;
    app->saved_green = 0;
    app->saved_blue = 0;

    app->button_queue = xQueueCreate(10, sizeof(uint32_t));

    if (app->button_queue == NULL) {
        ESP_LOGE(TAG, "No se pudo crear la cola del botón");
        free(app);
        return;
    }

    ret = configure_leds(app);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando LEDs: %s", esp_err_to_name(ret));
        return;
    }

    ret = analog_init(&app->analog, POT_ADC_CHANNEL, NTC_ADC_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando ADC: %s", esp_err_to_name(ret));
        return;
    }

    ret = button_init(BUTTON_GPIO, app->button_queue);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando botón: %s", esp_err_to_name(ret));
        return;
    }

    xTaskCreate(
        temperature_task,
        "temperature_task",
        4096,
        app,
        2,
        NULL
    );

    xTaskCreate(
        color_config_task,
        "color_config_task",
        4096,
        app,
        3,
        NULL
    );
}