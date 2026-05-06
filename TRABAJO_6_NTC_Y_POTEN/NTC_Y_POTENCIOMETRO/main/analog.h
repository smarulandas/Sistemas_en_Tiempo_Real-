#ifndef ANALOG_H
#define ANALOG_H

#include <stdint.h>
#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"

typedef struct {
    adc_oneshot_unit_handle_t adc_handle;
    adc_channel_t pot_channel;
    adc_channel_t ntc_channel;
} analog_t;

esp_err_t analog_init(analog_t *analog, adc_channel_t pot_channel, adc_channel_t ntc_channel);

esp_err_t analog_read_pot_duty(analog_t *analog, uint8_t *duty);

esp_err_t analog_read_ntc_temperature(analog_t *analog, float *temperature_c);

#endif