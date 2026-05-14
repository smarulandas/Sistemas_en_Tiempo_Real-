#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_err.h"

#include "board_config.h"
#include "led_rgb.h"
#include "analog.h"
#include "button.h"
#include "uart_cmd.h"

/*
 * Estados del LED RGB configurable.
 */
typedef enum {
    STATE_CONFIG_RED = 0,
    STATE_CONFIG_BLUE,
    STATE_CONFIG_GREEN,
    STATE_SHOW_COLOR
} color_state_t;

/*
 * Configuración de un color para el LED de temperatura.
 */
typedef struct {
    float min_c;
    float max_c;
    uint8_t intensity_percent;
} temp_color_config_t;

/*
 * Configuración completa del LED RGB de temperatura.
 */
typedef struct {
    temp_color_config_t red;
    temp_color_config_t blue;
    temp_color_config_t green;
} temp_led_config_t;

/*
 * Contexto general de la aplicación.
 */
typedef struct {
    led_rgb_t led_config;
    led_rgb_t led_temperature;

    analog_t analog;

    QueueHandle_t button_queue;
    QueueHandle_t uart_command_queue;

    SemaphoreHandle_t config_mutex;

    color_state_t current_state;

    uint8_t saved_red;
    uint8_t saved_green;
    uint8_t saved_blue;

    temp_led_config_t temp_config;

    float last_temperature_c;
} app_context_t;

static const char *TAG = "MAIN";

/*
 * Convierte porcentaje 0-100 a duty 0-255.
 
static uint8_t percent_to_duty(uint8_t percent)
{
    if (percent > 100) {
        percent = 100;
    }

    return (uint8_t)((percent * 255) / 100);
}


 * Convierte duty 0-255 a porcentaje 0-100.
 
static uint8_t duty_to_percent(uint8_t duty)
{
    return (uint8_t)((duty * 100) / 255);
}
*/

//De aqui el nuevo

/*
 * Convierte porcentaje 0-100 a duty 0-255.
 *
 * Se suma 50 para redondear correctamente y no solo truncar.
 *
 * Ejemplo:
 * 39% de 255 = 99.45
 * Con redondeo queda cerca de 99.
 */
static uint8_t percent_to_duty(uint8_t percent)
{
    if (percent > 100) {
        percent = 100;
    }

    return (uint8_t)(((uint16_t)percent * 255 + 50) / 100);
}

/*
 * Convierte duty 0-255 a porcentaje 0-100.
 *
 * Se suma 127 para redondear correctamente al convertir de nuevo.
 *
 * Esto evita que valores como 39% aparezcan como 38%.
 */
static uint8_t duty_to_percent(uint8_t duty)
{
    return (uint8_t)(((uint16_t)duty * 100 + 127) / 255);
}


/*
 * Limita un valor entero entre 0 y 100.
 */
static uint8_t clamp_percent(float value)
{
    if (value < 0.0f) {
        return 0;
    }

    if (value > 100.0f) {
        return 100;
    }

    return (uint8_t)value;
}

/*
 * Verifica si una temperatura está dentro de un rango.
 */
static bool temperature_in_range(float temperature_c, float min_c, float max_c)
{
    return (temperature_c >= min_c && temperature_c <= max_c);
}

/*
 * Convierte el estado actual a texto.
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
 * Imprime ayuda de comandos UART.
 */
static void print_uart_help(void)
{
    ESP_LOGI(TAG, "===== COMANDOS UART DISPONIBLES =====");
    ESP_LOGI(TAG, "LIM_MI_R valor   -> limite minimo Red");
    ESP_LOGI(TAG, "LIM_MA_R valor   -> limite maximo Red");
    ESP_LOGI(TAG, "LIM_MI_B valor   -> limite minimo Blue");
    ESP_LOGI(TAG, "LIM_MA_B valor   -> limite maximo Blue");
    ESP_LOGI(TAG, "LIM_MI_G valor   -> limite minimo Green");
    ESP_LOGI(TAG, "LIM_MA_G valor   -> limite maximo Green");
    ESP_LOGI(TAG, "INT_R valor      -> intensidad Red 0 a 100");
    ESP_LOGI(TAG, "INT_B valor      -> intensidad Blue 0 a 100");
    ESP_LOGI(TAG, "INT_G valor      -> intensidad Green 0 a 100");
    ESP_LOGI(TAG, "COLOR            -> muestra RGB configurado con potenciometro");
    ESP_LOGI(TAG, "INFO             -> muestra temperatura, limites e intensidades");
    ESP_LOGI(TAG, "HELP             -> muestra esta ayuda");
    ESP_LOGI(TAG, "Tambien sirve formato: LIM_MI_R_5, INT_R_80");
}

/*
 * Imprime configuración del LED de temperatura.
 */
static void print_temperature_config(app_context_t *app)
{
    temp_led_config_t cfg;
    float temp;

    // Protege el acceso concurrente a la configuración de temperatura
    xSemaphoreTake(app->config_mutex, portMAX_DELAY);
    cfg = app->temp_config;
    temp = app->last_temperature_c;
    xSemaphoreGive(app->config_mutex);

    ESP_LOGI(TAG, "===== CONFIGURACION LED TEMPERATURA =====");
    ESP_LOGI(TAG, "Temperatura actual: %.2f C", temp);

    ESP_LOGI(TAG, "RED   : %.2f C a %.2f C | Intensidad: %u%%",
             cfg.red.min_c,
             cfg.red.max_c,
             cfg.red.intensity_percent);

    ESP_LOGI(TAG, "BLUE  : %.2f C a %.2f C | Intensidad: %u%%",
             cfg.blue.min_c,
             cfg.blue.max_c,
             cfg.blue.intensity_percent);

    ESP_LOGI(TAG, "GREEN : %.2f C a %.2f C | Intensidad: %u%%",
             cfg.green.min_c,
             cfg.green.max_c,
             cfg.green.intensity_percent);
}

/*
 * Imprime el color guardado del LED controlado por potenciómetro.
 */
static void print_pot_led_values(app_context_t *app)
{
    uint8_t red_percent = duty_to_percent(app->saved_red);
    uint8_t green_percent = duty_to_percent(app->saved_green);
    uint8_t blue_percent = duty_to_percent(app->saved_blue);

    ESP_LOGI(TAG, "===== LED POTENCIOMETRO =====");
    ESP_LOGI(TAG, "Estado actual: %s", state_to_string(app->current_state));
    ESP_LOGI(TAG, "Rojo guardado : %u%%", red_percent);
    ESP_LOGI(TAG, "Azul guardado : %u%%", blue_percent);
    ESP_LOGI(TAG, "Verde guardado: %u%%", green_percent);
}

/*
 * Guarda el valor actual del potenciómetro según el estado.
 */
static void save_current_state_value(app_context_t *app, uint8_t duty)
{
    // Guarda la intensidad actual del canal activo antes de cambiar de modo
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
 */
static void go_to_next_state(app_context_t *app)
{
    // Avanza cíclicamente entre los cuatro estados del modo configurador
    app->current_state = (app->current_state + 1) % 4;

    ESP_LOGI(TAG, "Nuevo estado: %s", state_to_string(app->current_state));

    print_pot_led_values(app);
}

/*
 * Actualiza el LED RGB configurable.
 */
static void update_config_led_output(app_context_t *app, uint8_t pot_duty)
{
    // Ajusta el color del LED según el estado actual del modo de configuración
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
 * Aplica un comando recibido por UART.
 */
static void apply_uart_command(app_context_t *app, uart_command_t command)
{
    bool changed_temperature_config = false;

    // Procesa cada tipo de comando UART y actualiza la configuración de temperatura
    switch (command.type) {
        case UART_CMD_SET_LIM_MI_R:
            xSemaphoreTake(app->config_mutex, portMAX_DELAY);
            app->temp_config.red.min_c = command.value;
            xSemaphoreGive(app->config_mutex);
            changed_temperature_config = true;
            break;

        case UART_CMD_SET_LIM_MA_R:
            xSemaphoreTake(app->config_mutex, portMAX_DELAY);
            app->temp_config.red.max_c = command.value;
            xSemaphoreGive(app->config_mutex);
            changed_temperature_config = true;
            break;

        case UART_CMD_SET_LIM_MI_B:
            xSemaphoreTake(app->config_mutex, portMAX_DELAY);
            app->temp_config.blue.min_c = command.value;
            xSemaphoreGive(app->config_mutex);
            changed_temperature_config = true;
            break;

        case UART_CMD_SET_LIM_MA_B:
            xSemaphoreTake(app->config_mutex, portMAX_DELAY);
            app->temp_config.blue.max_c = command.value;
            xSemaphoreGive(app->config_mutex);
            changed_temperature_config = true;
            break;

        case UART_CMD_SET_LIM_MI_G:
            xSemaphoreTake(app->config_mutex, portMAX_DELAY);
            app->temp_config.green.min_c = command.value;
            xSemaphoreGive(app->config_mutex);
            changed_temperature_config = true;
            break;

        case UART_CMD_SET_LIM_MA_G:
            xSemaphoreTake(app->config_mutex, portMAX_DELAY);
            app->temp_config.green.max_c = command.value;
            xSemaphoreGive(app->config_mutex);
            changed_temperature_config = true;
            break;

        case UART_CMD_SET_INT_R:
            xSemaphoreTake(app->config_mutex, portMAX_DELAY);
            app->temp_config.red.intensity_percent = clamp_percent(command.value);
            xSemaphoreGive(app->config_mutex);
            changed_temperature_config = true;
            break;

        case UART_CMD_SET_INT_B:
            xSemaphoreTake(app->config_mutex, portMAX_DELAY);
            app->temp_config.blue.intensity_percent = clamp_percent(command.value);
            xSemaphoreGive(app->config_mutex);
            changed_temperature_config = true;
            break;

        case UART_CMD_SET_INT_G:
            xSemaphoreTake(app->config_mutex, portMAX_DELAY);
            app->temp_config.green.intensity_percent = clamp_percent(command.value);
            xSemaphoreGive(app->config_mutex);
            changed_temperature_config = true;
            break;

        case UART_CMD_PRINT_INFO:
            print_temperature_config(app);
            print_pot_led_values(app);
            break;

        case UART_CMD_PRINT_COLOR:
            print_pot_led_values(app);
            break;

        case UART_CMD_PRINT_HELP:
            print_uart_help();
            break;

        case UART_CMD_INVALID:
            ESP_LOGW(TAG, "Comando invalido. Escriba HELP para ver comandos.");
            break;

        default:
            break;
    }

    if (changed_temperature_config) {
        ESP_LOGI(TAG, "Comando aplicado correctamente");
        print_temperature_config(app);
    }
}

/*
 * Tarea que procesa comandos UART.
 */
static void uart_command_task(void *args)
{
    app_context_t *app = (app_context_t *)args;
    uart_command_t command;

    print_uart_help();

    while (1) {
        if (xQueueReceive(app->uart_command_queue, &command, portMAX_DELAY) == pdTRUE) {
            apply_uart_command(app, command);
        }
    }
}

/*
 * Tarea del LED RGB de temperatura.
 *
 * Lee el NTC, calcula:
 * - temperatura en °C
 * - voltaje del divisor NTC
 *
 * Luego decide qué colores del LED RGB de temperatura deben encenderse
 * según los rangos configurados por UART.
 */
static void temperature_task(void *args)
{
    app_context_t *app = (app_context_t *)args;

    while (1) {
        float temperature_c = 0.0f;
        float ntc_voltage_mv = 0.0f;

        /*
         * Lee el NTC y obtiene:
         * - temperature_c: temperatura calculada en °C
         * - ntc_voltage_mv: voltaje leído en el divisor del NTC, en mV
         */
        esp_err_t ret = analog_read_ntc_temperature_voltage(
            &app->analog,
            &temperature_c,
            &ntc_voltage_mv
        );

        if (ret == ESP_OK) {
            temp_led_config_t cfg;

            /*
             * Se protege el acceso a la configuración de temperatura
             * porque también puede ser modificada desde UART.
             */
            xSemaphoreTake(app->config_mutex, portMAX_DELAY);
            app->last_temperature_c = temperature_c;
            cfg = app->temp_config;
            xSemaphoreGive(app->config_mutex);

            /*
             * Duty real que se manda al PWM.
             * Va de 0 a 255.
             */
            uint8_t red_duty = 0;
            uint8_t green_duty = 0;
            uint8_t blue_duty = 0;

            /*
             * Porcentajes que se muestran en el monitor.
             * Estos van de 0 a 100%.
             */
            uint8_t red_display_percent = 0;
            uint8_t blue_display_percent = 0;
            uint8_t green_display_percent = 0;

            /*
             * Revisión del rango del color rojo.
             *
             * Si la temperatura está entre:
             * LIM_MI_R y LIM_MA_R,
             * entonces el rojo se enciende con la intensidad INT_R.
             */
            if (temperature_in_range(temperature_c, cfg.red.min_c, cfg.red.max_c)) {
                red_duty = percent_to_duty(cfg.red.intensity_percent);
                red_display_percent = cfg.red.intensity_percent;
            }

            /*
             * Revisión del rango del color azul.
             */
            if (temperature_in_range(temperature_c, cfg.blue.min_c, cfg.blue.max_c)) {
                blue_duty = percent_to_duty(cfg.blue.intensity_percent);
                blue_display_percent = cfg.blue.intensity_percent;
            }

            /*
             * Revisión del rango del color verde.
             */
            if (temperature_in_range(temperature_c, cfg.green.min_c, cfg.green.max_c)) {
                green_duty = percent_to_duty(cfg.green.intensity_percent);
                green_display_percent = cfg.green.intensity_percent;
            }

            /*
             * Actualiza físicamente el LED RGB de temperatura.
             */
            led_rgb_set_color(&app->led_temperature, red_duty, green_duty, blue_duty);

            /*
             * Imprime:
             * - Temperatura calculada
             * - Voltaje del NTC en voltios
             * - Intensidad de cada color
             */
            ESP_LOGI(
                TAG,
                "T=%.2f C | V_NTC=%.3f V | LED_TEMP: R=%u%% B=%u%% G=%u%%",
                temperature_c,
                ntc_voltage_mv / 1000.0f,
                red_display_percent,
                blue_display_percent,
                green_display_percent
            );
        }
        else {
            ESP_LOGE(TAG, "Error leyendo NTC: %s", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(TEMP_TASK_PERIOD_MS));
    }
}

/*
 * Tarea del LED RGB configurable con potenciómetro.
 */
static void color_config_task(void *args)
{
    app_context_t *app = (app_context_t *)args;

    uint32_t button_event = 0;
    TickType_t last_button_tick = 0;

    uint8_t last_printed_percent = 255;
    color_state_t last_printed_state = STATE_SHOW_COLOR;

    ESP_LOGI(TAG, "Estado inicial: %s", state_to_string(app->current_state));

    while (1) {
        uint8_t pot_duty = 0;

        esp_err_t ret = analog_read_pot_duty(&app->analog, &pot_duty);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error leyendo potenciometro: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(CONFIG_TASK_PERIOD_MS));
            continue;
        }

        while (xQueueReceive(app->button_queue, &button_event, 0) == pdTRUE) {
            TickType_t now = xTaskGetTickCount();

            if ((now - last_button_tick) >= pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS)) {
                // Guarda el valor antes de cambiar de estado, para mantener el color actual
                save_current_state_value(app, pot_duty);
                go_to_next_state(app);
                last_button_tick = now;
            }
        }

        update_config_led_output(app, pot_duty);

        uint8_t current_percent = duty_to_percent(pot_duty);

        if (app->current_state != last_printed_state ||
            abs((int)current_percent - (int)last_printed_percent) >= 5) {

            ESP_LOGI(
                TAG,
                "LED_POT | Estado=%s | Potenciometro=%u%% | R=%u%% B=%u%% G=%u%%",
                state_to_string(app->current_state),
                current_percent,
                duty_to_percent(app->saved_red),
                duty_to_percent(app->saved_blue),
                duty_to_percent(app->saved_green)
            );

            last_printed_percent = current_percent;
            last_printed_state = app->current_state;
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_TASK_PERIOD_MS));
    }
}

/*
 * Configura los dos LED RGB.
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
 * Carga la configuración inicial del LED de temperatura.
 */
static void load_default_temperature_config(app_context_t *app)
{
    app->temp_config.red.min_c = DEFAULT_LIM_MI_R_C;
    app->temp_config.red.max_c = DEFAULT_LIM_MA_R_C;
    app->temp_config.red.intensity_percent = DEFAULT_INT_R_PERCENT;

    app->temp_config.blue.min_c = DEFAULT_LIM_MI_B_C;
    app->temp_config.blue.max_c = DEFAULT_LIM_MA_B_C;
    app->temp_config.blue.intensity_percent = DEFAULT_INT_B_PERCENT;

    app->temp_config.green.min_c = DEFAULT_LIM_MI_G_C;
    app->temp_config.green.max_c = DEFAULT_LIM_MA_G_C;
    app->temp_config.green.intensity_percent = DEFAULT_INT_G_PERCENT;

    app->last_temperature_c = 0.0f;
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

    load_default_temperature_config(app);

    app->button_queue = xQueueCreate(10, sizeof(uint32_t));

    if (app->button_queue == NULL) {
        ESP_LOGE(TAG, "No se pudo crear la cola del boton");
        free(app);
        return;
    }

    app->uart_command_queue = xQueueCreate(10, sizeof(uart_command_t));

    if (app->uart_command_queue == NULL) {
        ESP_LOGE(TAG, "No se pudo crear la cola UART");
        free(app);
        return;
    }

    app->config_mutex = xSemaphoreCreateMutex();

    if (app->config_mutex == NULL) {
        ESP_LOGE(TAG, "No se pudo crear mutex de configuracion");
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
        ESP_LOGE(TAG, "Error configurando boton: %s", esp_err_to_name(ret));
        return;
    }

    ret = uart_cmd_init(app->uart_command_queue);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando UART: %s", esp_err_to_name(ret));
        return;
    }

    print_temperature_config(app);

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

    xTaskCreate(
        uart_command_task,
        "uart_command_task",
        4096,
        app,
        4,
        NULL
    );
}