#ifndef ANALOG_H
#define ANALOG_H

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"

/*
 * Estructura para manejar las entradas analógicas.
 *
 * adc_handle:
 * - Manejador general del ADC.
 *
 * pot_channel:
 * - Canal ADC del potenciómetro.
 *
 * ntc_channel:
 * - Canal ADC del NTC.
 *
 * pot_cali_handle:
 * - Calibración del canal del potenciómetro.
 *
 * ntc_cali_handle:
 * - Calibración del canal del NTC.
 *
 * pot_calibrated / ntc_calibrated:
 * - Indican si la calibración ADC se pudo activar.
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

esp_err_t analog_init(
    analog_t *analog,
    adc_channel_t pot_channel,
    adc_channel_t ntc_channel
);

esp_err_t analog_read_pot_duty(
    analog_t *analog,
    uint8_t *duty
);

esp_err_t analog_read_ntc_temperature(
    analog_t *analog,
    float *temperature_c
);

esp_err_t analog_read_ntc_temperature_voltage(
    analog_t *analog,
    float *temperature_c,
    float *voltage_mv
);

#endif