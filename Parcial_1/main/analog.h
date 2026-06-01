#ifndef ANALOG_H
#define ANALOG_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"

/*
 * ============================================================
 * ESTRUCTURA DE LA LIBRERÍA ANALÓGICA
 * ============================================================
 *
 * No usa variables globales.
 */
typedef struct {
    adc_oneshot_unit_handle_t adc_handle;

    adc_channel_t pot_channel;
    adc_channel_t ntc_channel;

    adc_cali_handle_t pot_cali_handle;
    adc_cali_handle_t ntc_cali_handle;

    bool pot_calibrated;
    bool ntc_calibrated;
} analog_t;

/*
 * Inicializa el ADC.
 */
esp_err_t analog_init(
    analog_t *analog,
    adc_channel_t pot_channel,
    adc_channel_t ntc_channel
);

/*
 * Lee el potenciómetro como duty 0-255.
 */
esp_err_t analog_read_pot_duty(
    analog_t *analog,
    uint8_t *duty
);

/*
 * Lee el potenciómetro como umbral 0-100.
 */
esp_err_t analog_read_pot_threshold_percent(
    analog_t *analog,
    uint8_t *threshold_percent
);

/*
 * Lee temperatura en °C.
 */
esp_err_t analog_read_ntc_temperature(
    analog_t *analog,
    float *temperature_c
);

/*
 * Lee temperatura y voltaje.
 */
esp_err_t analog_read_ntc_temperature_voltage(
    analog_t *analog,
    float *temperature_c,
    float *voltage_mv
);

#endif