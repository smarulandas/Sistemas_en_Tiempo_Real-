#include "analog.h"
#include "board_config.h"

#include <math.h>

#include "esp_log.h"
#include "esp_adc/adc_cali_scheme.h"

#define TAG "ANALOG"

#define ADC_UNIT_USED           ADC_UNIT_1
#define ADC_ATTENUATION_USED    ADC_ATTEN_DB_12
#define ADC_BITWIDTH_USED       ADC_BITWIDTH_12
#define ADC_MAX_RAW_VALUE       4095
#define ADC_SAMPLES             64

/*
 * ============================================================
 * CALIBRACIÓN DEL ADC
 * ============================================================
 *
 * Intenta activar la calibración del ADC.
 * Si no se puede, el programa usa fórmula aproximada.
 */
static bool analog_calibration_init(
    adc_unit_t unit,
    adc_channel_t channel,
    adc_atten_t attenuation,
    adc_cali_handle_t *out_handle
)
{
    adc_cali_handle_t handle = NULL;

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = attenuation,
        .bitwidth = ADC_BITWIDTH_USED
    };

    esp_err_t ret = adc_cali_create_scheme_curve_fitting(
        &cali_config,
        &handle
    );

    if (ret == ESP_OK) {
        *out_handle = handle;
        ESP_LOGI(TAG, "Calibracion ADC activada en canal %d", channel);
        return true;
    }

    *out_handle = NULL;

    ESP_LOGW(
        TAG,
        "No se pudo calibrar canal %d. Se usara conversion aproximada.",
        channel
    );

    return false;
}

/*
 * ============================================================
 * INICIALIZACIÓN DEL ADC
 * ============================================================
 */
esp_err_t analog_init(
    analog_t *analog,
    adc_channel_t pot_channel,
    adc_channel_t ntc_channel
)
{
    analog->pot_channel = pot_channel;
    analog->ntc_channel = ntc_channel;

    analog->pot_cali_handle = NULL;
    analog->ntc_cali_handle = NULL;

    analog->pot_calibrated = false;
    analog->ntc_calibrated = false;

    adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = ADC_UNIT_USED,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };

    esp_err_t ret = adc_oneshot_new_unit(
        &unit_config,
        &analog->adc_handle
    );

    if (ret != ESP_OK) {
        return ret;
    }

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTENUATION_USED,
        .bitwidth = ADC_BITWIDTH_USED
    };

    ret = adc_oneshot_config_channel(
        analog->adc_handle,
        pot_channel,
        &channel_config
    );

    if (ret != ESP_OK) {
        return ret;
    }

    ret = adc_oneshot_config_channel(
        analog->adc_handle,
        ntc_channel,
        &channel_config
    );

    if (ret != ESP_OK) {
        return ret;
    }

    analog->pot_calibrated = analog_calibration_init(
        ADC_UNIT_USED,
        pot_channel,
        ADC_ATTENUATION_USED,
        &analog->pot_cali_handle
    );

    analog->ntc_calibrated = analog_calibration_init(
        ADC_UNIT_USED,
        ntc_channel,
        ADC_ATTENUATION_USED,
        &analog->ntc_cali_handle
    );

    ESP_LOGI(TAG, "ADC inicializado correctamente");

    return ESP_OK;
}

/*
 * ============================================================
 * LECTURA PROMEDIADA DEL ADC
 * ============================================================
 *
 * Se hacen varias lecturas y se promedian para reducir ruido.
 */
static esp_err_t analog_read_raw_average(
    analog_t *analog,
    adc_channel_t channel,
    int *raw_average
)
{
    int raw_value = 0;
    int raw_sum = 0;

    for (int sample_index = 0; sample_index < ADC_SAMPLES; sample_index++) {
        esp_err_t ret = adc_oneshot_read(
            analog->adc_handle,
            channel,
            &raw_value
        );

        if (ret != ESP_OK) {
            return ret;
        }

        raw_sum += raw_value;
    }

    *raw_average = raw_sum / ADC_SAMPLES;

    return ESP_OK;
}

/*
 * ============================================================
 * CONVERSIÓN RAW A VOLTAJE
 * ============================================================
 */
static float analog_raw_to_voltage_mv(
    analog_t *analog,
    adc_channel_t channel,
    int raw_value
)
{
    int voltage_mv = 0;

    if (channel == analog->pot_channel && analog->pot_calibrated) {
        esp_err_t ret = adc_cali_raw_to_voltage(
            analog->pot_cali_handle,
            raw_value,
            &voltage_mv
        );

        if (ret == ESP_OK) {
            return (float)voltage_mv;
        }
    }

    if (channel == analog->ntc_channel && analog->ntc_calibrated) {
        esp_err_t ret = adc_cali_raw_to_voltage(
            analog->ntc_cali_handle,
            raw_value,
            &voltage_mv
        );

        if (ret == ESP_OK) {
            return (float)voltage_mv;
        }
    }

    return ((float)raw_value * ADC_REFERENCE_MV) / ADC_MAX_RAW_VALUE;
}

/*
 * ============================================================
 * LECTURA DEL POTENCIÓMETRO COMO DUTY
 * ============================================================
 *
 * Convierte el potenciómetro a un valor de 0 a 255.
 */
esp_err_t analog_read_pot_duty(
    analog_t *analog,
    uint8_t *duty
)
{
    int raw_average = 0;

    esp_err_t ret = analog_read_raw_average(
        analog,
        analog->pot_channel,
        &raw_average
    );

    if (ret != ESP_OK) {
        return ret;
    }

    if (raw_average < POT_ADC_MIN_RAW) {
        raw_average = POT_ADC_MIN_RAW;
    }

    if (raw_average > POT_ADC_MAX_RAW) {
        raw_average = POT_ADC_MAX_RAW;
    }

    int raw_range = POT_ADC_MAX_RAW - POT_ADC_MIN_RAW;

    if (raw_range <= 0) {
        *duty = 0;
        return ESP_OK;
    }

    int duty_value =
        ((raw_average - POT_ADC_MIN_RAW) * 255) / raw_range;

    if (duty_value < 0) {
        duty_value = 0;
    }

    if (duty_value > 255) {
        duty_value = 255;
    }

    *duty = (uint8_t)duty_value;

    return ESP_OK;
}

/*
 * ============================================================
 * LECTURA DEL POTENCIÓMETRO COMO UMBRAL
 * ============================================================
 *
 * Convierte el potenciómetro a un valor entre 0 y 100.
 *
 * Ese valor se interpreta como umbral de temperatura en °C.
 */
esp_err_t analog_read_pot_threshold_percent(
    analog_t *analog,
    uint8_t *threshold_percent
)
{
    uint8_t duty = 0;

    esp_err_t ret = analog_read_pot_duty(
        analog,
        &duty
    );

    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t percent =
        (((uint16_t)duty * 100U) + 127U) / 255U;

    if (percent > 100U) {
        percent = 100U;
    }

    *threshold_percent = (uint8_t)percent;

    return ESP_OK;
}

/*
 * ============================================================
 * CONVERSIÓN DEL VOLTAJE DEL NTC A TEMPERATURA
 * ============================================================
 *
 * Conexión usada:
 *
 * 3.3V ---- resistencia fija 10k ---- ADC ---- NTC ---- GND
 */
static float analog_ntc_voltage_to_temperature_c(
    float voltage_mv
)
{
    float vcc_mv = ADC_REFERENCE_MV;

    if (voltage_mv <= 1.0f) {
        voltage_mv = 1.0f;
    }

    if (voltage_mv >= vcc_mv - 1.0f) {
        voltage_mv = vcc_mv - 1.0f;
    }

    float ntc_resistance =
        NTC_FIXED_RESISTOR_OHMS * voltage_mv / (vcc_mv - voltage_mv);

    float inverse_temperature =
        (1.0f / NTC_NOMINAL_TEMPERATURE_K) +
        (1.0f / NTC_BETA_VALUE) *
        logf(ntc_resistance / NTC_NOMINAL_RESISTANCE_OHMS);

    float temperature_k = 1.0f / inverse_temperature;
    float temperature_c = temperature_k - 273.15f;

    temperature_c =
        (temperature_c * TEMP_CALIBRATION_GAIN) +
        TEMP_CALIBRATION_OFFSET_C;

    return temperature_c;
}

/*
 * ============================================================
 * LECTURA DE TEMPERATURA DEL NTC
 * ============================================================
 */
esp_err_t analog_read_ntc_temperature(
    analog_t *analog,
    float *temperature_c
)
{
    float voltage_mv = 0.0f;

    return analog_read_ntc_temperature_voltage(
        analog,
        temperature_c,
        &voltage_mv
    );
}

/*
 * ============================================================
 * LECTURA DE TEMPERATURA Y VOLTAJE DEL NTC
 * ============================================================
 */
esp_err_t analog_read_ntc_temperature_voltage(
    analog_t *analog,
    float *temperature_c,
    float *voltage_mv
)
{
    int raw_average = 0;

    esp_err_t ret = analog_read_raw_average(
        analog,
        analog->ntc_channel,
        &raw_average
    );

    if (ret != ESP_OK) {
        return ret;
    }

    *voltage_mv = analog_raw_to_voltage_mv(
        analog,
        analog->ntc_channel,
        raw_average
    );

    *temperature_c = analog_ntc_voltage_to_temperature_c(
        *voltage_mv
    );

    return ESP_OK;
}