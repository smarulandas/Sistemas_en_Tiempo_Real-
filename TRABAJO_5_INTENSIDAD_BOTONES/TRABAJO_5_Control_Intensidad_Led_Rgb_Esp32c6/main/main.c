// Librería para usar bool
#include <stdbool.h>

// Librería para usar tipos enteros de tamaño fijo
#include <stdint.h>

// Librería GPIO para configuración y lectura de botones
#include "driver/gpio.h"

// Librería para manejo de errores ESP-IDF
#include "esp_err.h"

// Librería para mostrar mensajes por monitor serial
#include "esp_log.h"

// Librería para obtener tiempo en microsegundos
#include "esp_timer.h"

// Librerías de FreeRTOS para retardos y tareas
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Incluimos nuestra propia librería del LED RGB
#include "rgb_led.h"

// -----------------------------------------------------------------------------
// CONSTANTES
// -----------------------------------------------------------------------------

// Tiempo de debounce del botón en milisegundos
#define DEBOUNCE_TIME_MS   30

// Paso de incremento de intensidad por cada pulsación
#define STEP_PERCENT       10

// Tiempo entre cada iteración del while principal
#define POLL_TIME_MS       10

// -----------------------------------------------------------------------------
// ESTRUCTURAS
// -----------------------------------------------------------------------------

// Estructura para manejar el estado de un botón
typedef struct {
    gpio_num_t pin;           // Pin GPIO donde está conectado el botón
    int last_raw_level;       // Última lectura sin filtrar
    int stable_level;         // Último estado estable después del debounce
    int64_t last_change_us;   // Tiempo del último cambio detectado
} button_t;

// Estructura general de la aplicación.
// Agrupa el LED RGB y los tres botones.
typedef struct {
    rgb_led_t led;
    button_t button_red;
    button_t button_green;
    button_t button_blue;
} app_context_t;

// Nombre usado en los mensajes del monitor serial
static const char *TAG = "RGB_APP";

// -----------------------------------------------------------------------------
// FUNCIONES INTERNAS
// -----------------------------------------------------------------------------

// Inicializa un botón como entrada con pull-up interno
static esp_err_t button_init(button_t *button, gpio_num_t pin)
{
    // Validamos puntero
    if (button == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Configuración del GPIO del botón
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),         // Seleccionamos el pin
        .mode = GPIO_MODE_INPUT,               // Modo entrada
        .pull_up_en = GPIO_PULLUP_ENABLE,      // Activamos pull-up interno
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Desactivamos pull-down
        .intr_type = GPIO_INTR_DISABLE         // Sin interrupción
    };

    // Aplicamos la configuración
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        return err;
    }

    // Inicializamos el estado interno del botón
    button->pin = pin;
    button->last_raw_level = 1;           // En pull-up, suelto = 1
    button->stable_level = 1;             // Estado estable inicial
    button->last_change_us = esp_timer_get_time(); // Tiempo actual

    return ESP_OK;
}

// Detecta una pulsación válida usando debounce.
// Devuelve true solo cuando se detecta una nueva pulsación.
static bool button_pressed_event(button_t *button)
{
    // Leemos el nivel actual del pin
    int raw_level = gpio_get_level(button->pin);

    // Obtenemos el tiempo actual en microsegundos
    int64_t now_us = esp_timer_get_time();

    // Si hubo cambio respecto a la lectura anterior, actualizamos referencia
    if (raw_level != button->last_raw_level) {
        button->last_raw_level = raw_level;
        button->last_change_us = now_us;
    }

    // Verificamos si el estado se ha mantenido estable por suficiente tiempo
    if ((now_us - button->last_change_us) >= (DEBOUNCE_TIME_MS * 1000LL)) {
        // Si cambió el estado estable, lo actualizamos
        if (button->stable_level != raw_level) {
            button->stable_level = raw_level;

            // Como usamos pull-up:
            // - suelto = 1
            // - presionado = 0
            if (button->stable_level == 0) {
                return true;
            }
        }
    }

    return false;
}

// Muestra por monitor serial el estado actual del LED RGB
static void log_rgb_status(const rgb_led_t *led)
{
    ESP_LOGI(TAG,
             "R=%u%%  G=%u%%  B=%u%%",
             rgb_led_get_percent(led, RGB_CHANNEL_RED),
             rgb_led_get_percent(led, RGB_CHANNEL_GREEN),
             rgb_led_get_percent(led, RGB_CHANNEL_BLUE));
}

// -----------------------------------------------------------------------------
// FUNCIÓN PRINCIPAL
// -----------------------------------------------------------------------------

void app_main(void)
{
    // Mensaje para confirmar que sí entró a app_main
    printf("ENTRE A APP_MAIN\n");

    // Creamos la estructura principal de la aplicación
    app_context_t app = {
        .led = {
            // Pines del LED RGB
            .pin_r = GPIO_NUM_2,
            .pin_g = GPIO_NUM_3,
            .pin_b = GPIO_NUM_6,

            // Canales PWM usados por cada color
            .channel_r = LEDC_CHANNEL_0,
            .channel_g = LEDC_CHANNEL_1,
            .channel_b = LEDC_CHANNEL_2,

            // Configuración general del PWM
            .timer = LEDC_TIMER_0,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = LEDC_TIMER_13_BIT,
            .pwm_freq_hz = 5000,

            // true porque el LED RGB es ánodo común
            .active_low = true,

            // Intensidad inicial apagada
            .red_percent = 0,
            .green_percent = 0,
            .blue_percent = 0
        }
    };

    // Inicializamos el PWM del LED RGB
    ESP_ERROR_CHECK(rgb_led_init(&app.led));

    // Inicializamos los tres botones
    ESP_ERROR_CHECK(button_init(&app.button_red, GPIO_NUM_7));
    ESP_ERROR_CHECK(button_init(&app.button_green, GPIO_NUM_0));
    ESP_ERROR_CHECK(button_init(&app.button_blue, GPIO_NUM_1));

    // Mensaje de arranque correcto
    ESP_LOGI(TAG, "Sistema iniciado");

    // Mostramos estado inicial del LED
    log_rgb_status(&app.led);

    // Bucle principal del programa
    while (true) {
        // Si se presiona el botón rojo, incrementamos el rojo
        if (button_pressed_event(&app.button_red)) {
            ESP_ERROR_CHECK(rgb_led_step_up(&app.led, RGB_CHANNEL_RED, STEP_PERCENT));
            ESP_LOGI(TAG, "Boton ROJO presionado");
            log_rgb_status(&app.led);
        }

        // Si se presiona el botón verde, incrementamos el verde
        if (button_pressed_event(&app.button_green)) {
            ESP_ERROR_CHECK(rgb_led_step_up(&app.led, RGB_CHANNEL_GREEN, STEP_PERCENT));
            ESP_LOGI(TAG, "Boton VERDE presionado");
            log_rgb_status(&app.led);
        }

        // Si se presiona el botón azul, incrementamos el azul
        if (button_pressed_event(&app.button_blue)) {
            ESP_ERROR_CHECK(rgb_led_step_up(&app.led, RGB_CHANNEL_BLUE, STEP_PERCENT));
            ESP_LOGI(TAG, "Boton AZUL presionado");
            log_rgb_status(&app.led);
        }

        // Pequeño retardo para no saturar la CPU
        vTaskDelay(pdMS_TO_TICKS(POLL_TIME_MS));
    }
}