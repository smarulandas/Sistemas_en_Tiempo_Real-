#include "analog.h"       // Funciones y estructura del módulo analógico.
#include "board_config.h" // Constantes del ADC, NTC y pines.
#include <math.h>         // Permite usar logf() para la ecuación Beta.
#include "esp_log.h"      // Permite imprimir mensajes.

#define ADC_UNIT_USED           ADC_UNIT_1       // Unidad ADC usada.
#define ADC_ATTENUATION_USED    ADC_ATTEN_DB_12  // Atenuación para leer hasta cerca de 3.3 V.
#define ADC_BITWIDTH_USED       ADC_BITWIDTH_12  // Resolución ADC de 12 bits.
#define ADC_MAX_RAW_VALUE       4095             // Valor máximo para 12 bits.
#define ADC_SAMPLES             64               // Muestras para promediar.

static const char *TAG = "ANALOG";               // Etiqueta para logs.

// Inicializa ADC y configura los canales del potenciómetro y NTC.
esp_err_t analog_init(analog_t *analog, adc_channel_t pot_channel, adc_channel_t ntc_channel)
{
    analog->pot_channel = pot_channel;           // Guarda canal del potenciómetro.
    analog->ntc_channel = ntc_channel;           // Guarda canal del NTC.

    adc_oneshot_unit_init_cfg_t unit_config = {  // Configuración de unidad ADC.
        .unit_id = ADC_UNIT_USED,                // ADC1.
        .ulp_mode = ADC_ULP_MODE_DISABLE         // No se usa ULP.
    };

    esp_err_t ret = adc_oneshot_new_unit(&unit_config, &analog->adc_handle); // Crea unidad ADC.
    if (ret != ESP_OK) return ret;               // Retorna error si falla.

    adc_oneshot_chan_cfg_t channel_config = {    // Configuración de canales ADC.
        .atten = ADC_ATTENUATION_USED,           // Atenuación 12 dB.
        .bitwidth = ADC_BITWIDTH_USED            // 12 bits.
    };

    ret = adc_oneshot_config_channel(analog->adc_handle, pot_channel, &channel_config); // Canal pot.
    if (ret != ESP_OK) return ret;

    ret = adc_oneshot_config_channel(analog->adc_handle, ntc_channel, &channel_config); // Canal NTC.
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "ADC configurado correctamente"); // Mensaje de confirmación.
    return ESP_OK;                                  // Correcto.
}

// Lee un canal ADC varias veces y promedia para reducir ruido.
static esp_err_t analog_read_raw_average(analog_t *analog, adc_channel_t channel, int *raw_average)
{
    int raw_value = 0;                              // Guarda una lectura.
    int sum = 0;                                    // Acumula lecturas.

    for (int i = 0; i < ADC_SAMPLES; i++) {         // Repite varias veces.
        esp_err_t ret = adc_oneshot_read(analog->adc_handle, channel, &raw_value); // Lee ADC.
        if (ret != ESP_OK) return ret;              // Retorna si falla.
        sum += raw_value;                           // Suma lectura.
    }

    *raw_average = sum / ADC_SAMPLES;               // Calcula promedio.
    return ESP_OK;                                  // Correcto.
}

// Lee el potenciómetro y lo escala de ADC 0-4095 a PWM 0-255.
esp_err_t analog_read_pot_duty(analog_t *analog, uint8_t *duty)
{
    int raw_average = 0;                            // Promedio ADC.

    esp_err_t ret = analog_read_raw_average(analog, analog->pot_channel, &raw_average); // Lee pot.
    if (ret != ESP_OK) return ret;

    if (raw_average < 0) raw_average = 0;            // Límite inferior.
    if (raw_average > ADC_MAX_RAW_VALUE) raw_average = ADC_MAX_RAW_VALUE; // Límite superior.

    *duty = (uint8_t)((raw_average * 255) / ADC_MAX_RAW_VALUE); // Escala a 0-255.
    return ESP_OK;                                  // Correcto.
}

// Convierte ADC crudo a milivoltios usando V = raw * Vref / 4095.
static float analog_raw_to_voltage_mv(int raw_value)
{
    return ((float)raw_value * ADC_REFERENCE_MV) / ADC_MAX_RAW_VALUE; // Voltaje aproximado.
}

// Convierte voltaje del divisor del NTC a temperatura en °C.
static float analog_ntc_voltage_to_temperature(float voltage_mv)
{
    float vcc_mv = ADC_REFERENCE_MV;                // VCC aproximado en mV.

    if (voltage_mv <= 1.0f) voltage_mv = 1.0f;       // Evita división por 0.
    if (voltage_mv >= vcc_mv - 1.0f) voltage_mv = vcc_mv - 1.0f; // Evita VCC exacto.

    // Fórmula del divisor:
    // 3.3V -- R fija -- ADC -- NTC -- GND
    // R_NTC = R_FIJA * V_ADC / (VCC - V_ADC)
    float ntc_resistance =
        NTC_FIXED_RESISTOR_OHMS * voltage_mv / (vcc_mv - voltage_mv);

    // Ecuación Beta:
    // 1/T = 1/T0 + (1/B) * ln(R/R0)
    float inverse_temperature =
        (1.0f / NTC_NOMINAL_TEMPERATURE_K) +
        (1.0f / NTC_BETA_VALUE) *
        logf(ntc_resistance / NTC_NOMINAL_RESISTANCE_OHMS);

    float temperature_k = 1.0f / inverse_temperature; // Temperatura en Kelvin.
    float temperature_c = temperature_k - 273.15f;    // Conversión a Celsius.

    return temperature_c;                           // Devuelve °C.
}

// Lee el NTC y calcula temperatura.
esp_err_t analog_read_ntc_temperature(analog_t *analog, float *temperature_c)
{
    int raw_average = 0;                            // Promedio ADC del NTC.

    esp_err_t ret = analog_read_raw_average(analog, analog->ntc_channel, &raw_average); // Lee NTC.
    if (ret != ESP_OK) return ret;

    float voltage_mv = analog_raw_to_voltage_mv(raw_average); // ADC a voltaje.
    *temperature_c = analog_ntc_voltage_to_temperature(voltage_mv); // Voltaje a °C.

    return ESP_OK;                                  // Correcto.
}
