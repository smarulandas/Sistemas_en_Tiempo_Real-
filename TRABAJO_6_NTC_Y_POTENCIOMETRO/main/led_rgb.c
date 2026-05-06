#include "led_rgb.h"       // Incluye la estructura y funciones del LED RGB.
#include "esp_log.h"        // Permite imprimir mensajes en el monitor serial.

#define LEDC_TIMER              LEDC_TIMER_0          // Temporizador PWM usado.
#define LEDC_MODE               LEDC_LOW_SPEED_MODE   // Modo LEDC usado en ESP32-C6.
#define LEDC_DUTY_RESOLUTION    LEDC_TIMER_8_BIT      // Resolución de 8 bits: 0 a 255.
#define LEDC_FREQUENCY_HZ       4000                  // Frecuencia PWM de 4 kHz.

static const char *TAG = "LED_RGB";                   // Etiqueta para logs.

// Invierte el duty si el LED es de ánodo común.
static uint32_t led_rgb_apply_polarity(const led_rgb_t *led, uint8_t value)
{
    if (led->common_anode) {       // Si el común del LED está a 3.3 V...
        return 255 - value;        // ...se invierte la lógica del PWM.
    }
    return value;                  // Para cátodo común se conserva el valor.
}

// Configura el temporizador general del PWM.
esp_err_t led_rgb_driver_init(void)
{
    ledc_timer_config_t timer_config = {              // Estructura de configuración LEDC.
        .speed_mode = LEDC_MODE,                      // Modo de velocidad.
        .timer_num = LEDC_TIMER,                      // Temporizador seleccionado.
        .duty_resolution = LEDC_DUTY_RESOLUTION,      // Resolución PWM.
        .freq_hz = LEDC_FREQUENCY_HZ,                 // Frecuencia de PWM.
        .clk_cfg = LEDC_AUTO_CLK                      // Reloj automático.
    };

    esp_err_t ret = ledc_timer_config(&timer_config); // Aplica configuración.
    if (ret == ESP_OK) {                              // Si fue correcto...
        ESP_LOGI(TAG, "Temporizador LEDC configurado correctamente");
    }
    return ret;                                       // Devuelve estado.
}

// Configura un canal PWM asociado a un GPIO.
static esp_err_t led_rgb_config_channel(gpio_num_t pin, ledc_channel_t channel)
{
    ledc_channel_config_t channel_config = {          // Estructura del canal PWM.
        .gpio_num = pin,                              // Pin físico.
        .speed_mode = LEDC_MODE,                      // Modo LEDC.
        .channel = channel,                           // Canal PWM.
        .intr_type = LEDC_INTR_DISABLE,               // Sin interrupciones PWM.
        .timer_sel = LEDC_TIMER,                      // Temporizador usado.
        .duty = 0,                                    // Duty inicial.
        .hpoint = 0                                   // Punto inicial del pulso.
    };

    return ledc_channel_config(&channel_config);      // Configura el canal.
}

// Inicializa los tres canales de un LED RGB.
esp_err_t led_rgb_init(led_rgb_t *led)
{
    esp_err_t ret;                                    // Variable de error.

    ret = led_rgb_config_channel(led->pin_r, led->channel_r); // Canal rojo.
    if (ret != ESP_OK) return ret;

    ret = led_rgb_config_channel(led->pin_g, led->channel_g); // Canal verde.
    if (ret != ESP_OK) return ret;

    ret = led_rgb_config_channel(led->pin_b, led->channel_b); // Canal azul.
    if (ret != ESP_OK) return ret;

    return led_rgb_off(led);                          // Inicia apagado.
}

// Cambia el color RGB usando valores de 0 a 255.
esp_err_t led_rgb_set_color(led_rgb_t *led, uint8_t red, uint8_t green, uint8_t blue)
{
    esp_err_t ret;                                    // Variable para errores.

    ret = ledc_set_duty(LEDC_MODE, led->channel_r, led_rgb_apply_polarity(led, red)); // Duty rojo.
    if (ret != ESP_OK) return ret;
    ret = ledc_update_duty(LEDC_MODE, led->channel_r); // Aplica duty rojo.
    if (ret != ESP_OK) return ret;

    ret = ledc_set_duty(LEDC_MODE, led->channel_g, led_rgb_apply_polarity(led, green)); // Duty verde.
    if (ret != ESP_OK) return ret;
    ret = ledc_update_duty(LEDC_MODE, led->channel_g); // Aplica duty verde.
    if (ret != ESP_OK) return ret;

    ret = ledc_set_duty(LEDC_MODE, led->channel_b, led_rgb_apply_polarity(led, blue)); // Duty azul.
    if (ret != ESP_OK) return ret;
    ret = ledc_update_duty(LEDC_MODE, led->channel_b); // Aplica duty azul.
    if (ret != ESP_OK) return ret;

    return ESP_OK;                                    // Todo correcto.
}

// Apaga el LED RGB.
esp_err_t led_rgb_off(led_rgb_t *led)
{
    return led_rgb_set_color(led, 0, 0, 0);           // Los tres colores en 0.
}
