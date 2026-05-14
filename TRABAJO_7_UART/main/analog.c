#include "analog.h"
#include "board_config.h"

#include <math.h>
#include <stdio.h>

#include "esp_log.h"
#include "esp_adc/adc_cali_scheme.h"

#define ADC_UNIT_USED           ADC_UNIT_1
#define ADC_ATTENUATION_USED    ADC_ATTEN_DB_12
#define ADC_BITWIDTH_USED       ADC_BITWIDTH_12
#define ADC_MAX_RAW_VALUE       4095
#define ADC_SAMPLES             64

static const char *TAG = "ANALOG";

/*
 * ============================================================
 * CALIBRACIÓN DEL ADC
 * ============================================================
 *
 * Antes se usaba esta conversión aproximada:
 *
 * V_ADC = raw * 3300 / 4095
 *
 * Pero esa fórmula puede tener error en el ESP32-C6.
 *
 * Ahora se usa el driver de calibración ADC de ESP-IDF:
 *
 * adc_cali_raw_to_voltage()
 *
 * Esto hace que el voltaje calculado por el programa sea más parecido
 * al voltaje real medido con multímetro.
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

    ESP_LOGW(
        TAG,
        "No se pudo activar calibracion ADC en canal %d. Se usara formula aproximada.",
        channel
    );

    *out_handle = NULL;
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

    /*
     * Se activa calibración independiente para cada canal.
     */
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

    ESP_LOGI(TAG, "ADC configurado correctamente");

    return ESP_OK;
}

/*
 * ============================================================
 * LECTURA PROMEDIADA DEL ADC
 * ============================================================
 *
 * Se hacen varias lecturas y se calcula el promedio para reducir ruido.
 */
static esp_err_t analog_read_raw_average(
    analog_t *analog,
    adc_channel_t channel,
    int *raw_average
)
{
    int raw_value = 0;
    int sum = 0;

    for (int i = 0; i < ADC_SAMPLES; i++) {
        esp_err_t ret = adc_oneshot_read(
            analog->adc_handle,
            channel,
            &raw_value
        );

        if (ret != ESP_OK) {
            return ret;
        }

        sum += raw_value;
    }

    *raw_average = sum / ADC_SAMPLES;

    return ESP_OK;
}

/*
 * ============================================================
 * CONVERSIÓN ADC A VOLTAJE CALIBRADO
 * ============================================================
 *
 * Si la calibración está disponible:
 * - usa adc_cali_raw_to_voltage().
 *
 * Si no está disponible:
 * - usa la fórmula aproximada antigua.
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

    /*
     * Respaldo si no se pudo calibrar.
     * Esta es la fórmula aproximada anterior.
     */
    return ((float)raw_value * ADC_REFERENCE_MV) / ADC_MAX_RAW_VALUE;
}

/*
 * ============================================================
 * LECTURA DEL POTENCIÓMETRO
 * ============================================================
 *
 * Convierte el valor ADC del potenciómetro en duty PWM de 0 a 255.
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

    /*
     * Si agregaste calibración de rango del potenciómetro en board_config.h,
     * usa POT_ADC_MIN_RAW y POT_ADC_MAX_RAW.
     *
     * Si no los tienes definidos, puedes dejar estos valores normales.
     */
#ifndef POT_ADC_MIN_RAW
#define POT_ADC_MIN_RAW 0
#endif

#ifndef POT_ADC_MAX_RAW
#define POT_ADC_MAX_RAW ADC_MAX_RAW_VALUE
#endif

    if (raw_average < POT_ADC_MIN_RAW) {
        raw_average = POT_ADC_MIN_RAW;
    }

    if (raw_average > POT_ADC_MAX_RAW) {
        raw_average = POT_ADC_MAX_RAW;
    }

    int calibrated_range = POT_ADC_MAX_RAW - POT_ADC_MIN_RAW;

    if (calibrated_range <= 0) {
        *duty = 0;
        return ESP_OK;
    }

    int duty_value =
        ((raw_average - POT_ADC_MIN_RAW) * 255) / calibrated_range;

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
 * CONVERSIÓN DE VOLTAJE A TEMPERATURA
 * ============================================================
 *
 * Conexión usada:
 *
 * 3.3V ---- R fija 10k ---- ADC ---- NTC 10k ---- GND
 *
 * Fórmula del divisor:
 *
 * R_NTC = R_FIJA * V_ADC / (VCC - V_ADC)
 *
 * Luego se aplica la ecuación Beta:
 *
 * 1/T = 1/T0 + (1/B) * ln(R/R0)
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

    float ntc_resistance =
        NTC_FIXED_RESISTOR_OHMS * voltage_mv / (vcc_mv - voltage_mv);

    float inverse_temperature =
        (1.0f / NTC_NOMINAL_TEMPERATURE_K) +
        (1.0f / NTC_BETA_VALUE) *
        logf(ntc_resistance / NTC_NOMINAL_RESISTANCE_OHMS);

    float temperature_k = 1.0f / inverse_temperature;
    float temperature_c = temperature_k - 273.15f;

#ifdef TEMP_CALIBRATION_GAIN
    temperature_c = temperature_c * TEMP_CALIBRATION_GAIN;
#endif

#ifdef TEMP_CALIBRATION_OFFSET_C
    temperature_c = temperature_c + TEMP_CALIBRATION_OFFSET_C;
#endif

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
 *
 * Esta función entrega:
 * - temperature_c: temperatura calculada.
 * - voltage_mv: voltaje calibrado del ADC en milivoltios.
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

    *temperature_c = analog_ntc_voltage_to_temperature(*voltage_mv);

    return ESP_OK;
}