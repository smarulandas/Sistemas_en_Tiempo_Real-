#ifndef ANALOG_H
#define ANALOG_H

#include <stdint.h>                    // Permite usar uint8_t.
#include "esp_err.h"                   // Permite usar esp_err_t.
#include "esp_adc/adc_oneshot.h"       // Driver ADC oneshot.

// Estructura para manejar el ADC y sus canales.
typedef struct {
    adc_oneshot_unit_handle_t adc_handle; // Manejador de la unidad ADC.
    adc_channel_t pot_channel;            // Canal del potenciómetro.
    adc_channel_t ntc_channel;            // Canal del NTC.
} analog_t;

esp_err_t analog_init(analog_t *analog, adc_channel_t pot_channel, adc_channel_t ntc_channel); // Inicializa ADC.
esp_err_t analog_read_pot_duty(analog_t *analog, uint8_t *duty); // Lee potenciómetro y devuelve duty 0-255.
esp_err_t analog_read_ntc_temperature(analog_t *analog, float *temperature_c); // Lee NTC y devuelve °C.

#endif
