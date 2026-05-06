#include "analog.h"
#include "board_config.h"
#include <math.h>
#include "esp_log.h"

#define ADC_UNIT_USED           ADC_UNIT_1
#define ADC_ATTENUATION_USED    ADC_ATTEN_DB_12
#define ADC_BITWIDTH_USED       ADC_BITWIDTH_12
#define ADC_MAX_RAW_VALUE       4095
#define ADC_SAMPLES             64

static const char *TAG = "ANALOG";

/*
 * Inicializa el ADC en modo oneshot.
 */
esp_err_t analog_init(analog_t *analog, adc_channel_t pot_channel, adc_channel_t ntc_channel)
{
    analog->pot_channel = pot_channel;
    analog->ntc_channel = ntc_channel;

    adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = ADC_UNIT_USED,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };

    esp_err_t ret = adc_oneshot_new_unit(&unit_config, &analog->adc_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTENUATION_USED,
        .bitwidth = ADC_BITWIDTH_USED
    };

    ret = adc_oneshot_config_channel(analog->adc_handle, pot_channel, &channel_config);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = adc_oneshot_config_channel(analog->adc_handle, ntc_channel, &channel_config);
    if (ret != ESP_OK) {
        return ret;
    }

    ESP_LOGI(TAG, "ADC configurado correctamente");

    return ESP_OK;
}

/*
 * Lee un canal ADC varias veces y promedia el resultado.
 */
static esp_err_t analog_read_raw_average(analog_t *analog, adc_channel_t channel, int *raw_average)
{
    int raw_value = 0;
    int sum = 0;

    for (int i = 0; i < ADC_SAMPLES; i++) {
        esp_err_t ret = adc_oneshot_read(analog->adc_handle, channel, &raw_value);

        if (ret != ESP_OK) {
            return ret;
        }

        sum += raw_value;
    }

    *raw_average = sum / ADC_SAMPLES;

    return ESP_OK;
}

/*
 * Lee el potenciómetro y lo convierte a un valor PWM de 0 a 255.
 */
esp_err_t analog_read_pot_duty(analog_t *analog, uint8_t *duty)
{
    int raw_average = 0;

    esp_err_t ret = analog_read_raw_average(analog, analog->pot_channel, &raw_average);
    if (ret != ESP_OK) {
        return ret;
    }

    if (raw_average < 0) {
        raw_average = 0;
    }

    if (raw_average > ADC_MAX_RAW_VALUE) {
        raw_average = ADC_MAX_RAW_VALUE;
    }

    *duty = (uint8_t)((raw_average * 255) / ADC_MAX_RAW_VALUE);

    return ESP_OK;
}

/*
 * Convierte el valor crudo del ADC a voltaje aproximado.
 */
static float analog_raw_to_voltage_mv(int raw_value)
{
    return ((float)raw_value * ADC_REFERENCE_MV) / ADC_MAX_RAW_VALUE;
}

/*
 * Convierte el voltaje del divisor con NTC a temperatura.
 *
 * Suposición del divisor:
 *
 *      3.3 V ---- R fija ---- punto ADC ---- NTC ---- GND
 *
 * Fórmula:
 *
 *      R_NTC = R_FIJA * V_ADC / (VCC - V_ADC)
 *
 * Luego se usa la ecuación Beta:
 *
 *      1/T = 1/T0 + (1/B) * ln(R/R0)
 */
static float analog_ntc_voltage_to_temperature(float voltage_mv)
{
    float vcc_mv = ADC_REFERENCE_MV;

    if (voltage_mv <= 1.0f) {
        voltage_mv = 1.0f;
    }

    if (voltage_mv >= vcc_mv - 1.0f) {
        voltage_mv = vcc_mv - 1.0f;
    }

    float ntc_resistance = NTC_FIXED_RESISTOR_OHMS * voltage_mv / (vcc_mv - voltage_mv);

    float inverse_temperature =
        (1.0f / NTC_NOMINAL_TEMPERATURE_K) +
        (1.0f / NTC_BETA_VALUE) *
        logf(ntc_resistance / NTC_NOMINAL_RESISTANCE_OHMS);

    float temperature_k = 1.0f / inverse_temperature;
    float temperature_c = temperature_k - 273.15f;

    return temperature_c;
}

/*
 * Lee el NTC y devuelve la temperatura en °C.
 */
esp_err_t analog_read_ntc_temperature(analog_t *analog, float *temperature_c)
{
    int raw_average = 0;

    esp_err_t ret = analog_read_raw_average(analog, analog->ntc_channel, &raw_average);
    if (ret != ESP_OK) {
        return ret;
    }

    float voltage_mv = analog_raw_to_voltage_mv(raw_average);

    *temperature_c = analog_ntc_voltage_to_temperature(voltage_mv);

    return ESP_OK;
}