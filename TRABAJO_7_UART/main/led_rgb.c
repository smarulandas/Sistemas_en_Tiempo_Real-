#include "led_rgb.h"  // Incluye el archivo de cabecera para las definiciones del LED RGB
#include "esp_log.h"  // Incluye el archivo de cabecera para el logging de ESP

#define LEDC_TIMER              LEDC_TIMER_0  // Define el temporizador LEDC a utilizar (temporizador 0)
#define LEDC_MODE               LEDC_LOW_SPEED_MODE  // Define el modo de velocidad del LEDC (modo de baja velocidad)
#define LEDC_DUTY_RESOLUTION    LEDC_TIMER_8_BIT  // Define la resolución del duty cycle (8 bits)
#define LEDC_FREQUENCY_HZ       4000  // Define la frecuencia del PWM en Hz (4000 Hz)

static const char *TAG = "LED_RGB";  // Etiqueta para los logs del módulo LED RGB

/*
 * Aplica inversión si el LED RGB es de ánodo común.
 *
 * En ánodo común:
 * - La pata común va a 3.3 V.
 * - El LED prende cuando el GPIO baja.
 *
 * Por eso:
 * - 255 se convierte en 0.
 * - 0 se convierte en 255.
 */
static uint32_t led_rgb_apply_polarity(const led_rgb_t *led, uint8_t value)  // Función estática para aplicar polaridad al valor del LED
{
    if (led->common_anode) {  // Si el LED es de ánodo común
        return 255 - value;  // Invierte el valor (255 - value)
    }

    return value;  // Si no es ánodo común, devuelve el valor original
}

/*
 * Configura el temporizador PWM LEDC.
 */
esp_err_t led_rgb_driver_init(void)  // Función para inicializar el driver del LED RGB
{
    ledc_timer_config_t timer_config = {  // Estructura de configuración del temporizador
        .speed_mode = LEDC_MODE,  // Modo de velocidad
        .timer_num = LEDC_TIMER,  // Número del temporizador
        .duty_resolution = LEDC_DUTY_RESOLUTION,  // Resolución del duty
        .freq_hz = LEDC_FREQUENCY_HZ,  // Frecuencia en Hz
        .clk_cfg = LEDC_AUTO_CLK  // Configuración del reloj automática
    };

    esp_err_t ret = ledc_timer_config(&timer_config);  // Configura el temporizador y obtiene el resultado

    if (ret == ESP_OK) {  // Si la configuración fue exitosa
        ESP_LOGI(TAG, "Temporizador LEDC configurado");  // Log de información
    }

    return ret;  // Devuelve el resultado de la configuración
}

/*
 * Configura un canal PWM para un color.
 */
static esp_err_t led_rgb_config_channel(gpio_num_t pin, ledc_channel_t channel)  // Función estática para configurar un canal PWM
{
    ledc_channel_config_t channel_config = {  // Estructura de configuración del canal
        .gpio_num = pin,  // Número del GPIO
        .speed_mode = LEDC_MODE,  // Modo de velocidad
        .channel = channel,  // Canal LEDC
        .intr_type = LEDC_INTR_DISABLE,  // Deshabilita interrupciones
        .timer_sel = LEDC_TIMER,  // Selecciona el temporizador
        .duty = 0,  // Duty inicial en 0
        .hpoint = 0  // Punto alto inicial en 0
    };

    return ledc_channel_config(&channel_config);  // Configura el canal y devuelve el resultado
}

/*
 * Inicializa los tres canales de un LED RGB.
 */
esp_err_t led_rgb_init(led_rgb_t *led)  // Función para inicializar el LED RGB
{
    esp_err_t ret;  // Variable para almacenar el resultado de las operaciones

    ret = led_rgb_config_channel(led->pin_r, led->channel_r);  // Configura el canal para el rojo
    if (ret != ESP_OK) {  // Si falla
        return ret;  // Devuelve el error
    }

    ret = led_rgb_config_channel(led->pin_g, led->channel_g);  // Configura el canal para el verde
    if (ret != ESP_OK) {  // Si falla
        return ret;  // Devuelve el error
    }

    ret = led_rgb_config_channel(led->pin_b, led->channel_b);  // Configura el canal para el azul
    if (ret != ESP_OK) {  // Si falla
        return ret;  // Devuelve el error
    }

    return led_rgb_off(led);  // Apaga el LED y devuelve el resultado
}

/*
 * Cambia el color del LED RGB.
 *
 * red, green y blue van de 0 a 255.
 */
esp_err_t led_rgb_set_color(led_rgb_t *led, uint8_t red, uint8_t green, uint8_t blue)  // Función para establecer el color del LED
{
    esp_err_t ret;  // Variable para el resultado

    ret = ledc_set_duty(LEDC_MODE, led->channel_r, led_rgb_apply_polarity(led, red));  // Establece el duty para el rojo aplicando polaridad
    if (ret != ESP_OK) {  // Si falla
        return ret;  // Devuelve el error
    }

    ret = ledc_update_duty(LEDC_MODE, led->channel_r);  // Actualiza el duty para el rojo
    if (ret != ESP_OK) {  // Si falla
        return ret;  // Devuelve el error
    }

    ret = ledc_set_duty(LEDC_MODE, led->channel_g, led_rgb_apply_polarity(led, green));  // Establece el duty para el verde aplicando polaridad
    if (ret != ESP_OK) {  // Si falla
        return ret;  // Devuelve el error
    }

    ret = ledc_update_duty(LEDC_MODE, led->channel_g);  // Actualiza el duty para el verde
    if (ret != ESP_OK) {  // Si falla
        return ret;  // Devuelve el error
    }

    ret = ledc_set_duty(LEDC_MODE, led->channel_b, led_rgb_apply_polarity(led, blue));  // Establece el duty para el azul aplicando polaridad
    if (ret != ESP_OK) {  // Si falla
        return ret;  // Devuelve el error
    }

    ret = ledc_update_duty(LEDC_MODE, led->channel_b);  // Actualiza el duty para el azul
    if (ret != ESP_OK) {  // Si falla
        return ret;  // Devuelve el error
    }

    return ESP_OK;  // Devuelve éxito
}

/*
 * Apaga el LED RGB.
 */
esp_err_t led_rgb_off(led_rgb_t *led)  // Función para apagar el LED RGB
{
    return led_rgb_set_color(led, 0, 0, 0);  // Establece el color a negro (apagado)
}