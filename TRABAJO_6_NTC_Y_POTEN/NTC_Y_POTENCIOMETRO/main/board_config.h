#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "driver/gpio.h"
#include "hal/adc_types.h"

/*
 * ============================================================
 * CONFIGURACIÓN GENERAL DEL HARDWARE
 * ============================================================
 *
 * Si tu LED RGB es de cátodo común:
 *      RGB_COMMON_ANODE = false
 *
 * Si tu LED RGB es de ánodo común:
 *      RGB_COMMON_ANODE = true
 */
#define RGB_COMMON_ANODE true
/*
 * ============================================================
 * PINES DEL LED RGB CONFIGURABLE
 * ============================================================
 *
 * Este LED RGB es el del Punto 2:
 * - Config Red
 * - Config Blue
 * - Config Green
 * - Show Color
 */
#define LED_CONFIG_R_GPIO      GPIO_NUM_4
#define LED_CONFIG_G_GPIO      GPIO_NUM_5
#define LED_CONFIG_B_GPIO      GPIO_NUM_6

/*
 * ============================================================
 * PINES DEL LED RGB DE TEMPERATURA
 * ============================================================
 *
 * Este LED RGB es el del Punto 1:
 * - Azul  si T < 25 °C
 * - Verde si 25 °C <= T <= 35 °C
 * - Rojo  si T > 35 °C
 */
#define LED_TEMP_R_GPIO        GPIO_NUM_10
#define LED_TEMP_G_GPIO        GPIO_NUM_18
#define LED_TEMP_B_GPIO        GPIO_NUM_19

/*
 * ============================================================
 * BOTÓN
 * ============================================================
 *
 * El botón cambia de estado:
 * CONFIG_RED → CONFIG_BLUE → CONFIG_GREEN → SHOW_COLOR → CONFIG_RED
 *
 * Conexión recomendada:
 * - Un lado del botón a GPIO7
 * - El otro lado del botón a GND
 * - Se usa pull-up interno
 */
#define BUTTON_GPIO            GPIO_NUM_7

/*
 * ============================================================
 * ADC
 * ============================================================
 *
 * POT_ADC_CHANNEL:
 *      Canal ADC para el potenciómetro.
 *
 * NTC_ADC_CHANNEL:
 *      Canal ADC para el divisor de tensión con NTC.
 *
 * Nota:
 * En ESP32-C6 los canales ADC dependen del pin físico usado.
 * Si cambias el pin, cambia también el canal ADC.
 */
#define POT_ADC_CHANNEL        ADC_CHANNEL_0
#define NTC_ADC_CHANNEL        ADC_CHANNEL_1

/*
 * ============================================================
 * CONFIGURACIÓN DEL NTC
 * ============================================================
 *
 * Para un NTC de 10k:
 *      NTC_NOMINAL_RESISTANCE_OHMS = 10000.0f
 *
 * Para un NTC de 4k:
 *      NTC_NOMINAL_RESISTANCE_OHMS = 4000.0f
 *
 * NTC_FIXED_RESISTOR_OHMS:
 *      Es la resistencia fija del divisor de tensión.
 *      Normalmente se usa 10k.
 */
#define NTC_NOMINAL_RESISTANCE_OHMS     10000.0f
#define NTC_FIXED_RESISTOR_OHMS         10000.0f
#define NTC_BETA_VALUE                  3950.0f
#define NTC_NOMINAL_TEMPERATURE_K       298.15f

/*
 * Voltaje de referencia usado para aproximar la conversión.
 * Para un proyecto de clase está bien usar 3.3 V.
 */
#define ADC_REFERENCE_MV                3300.0f

/*
 * ============================================================
 * UMBRALES DE TEMPERATURA
 * ============================================================
 */
#define TEMP_BLUE_LIMIT_C               25.0f
#define TEMP_RED_LIMIT_C                35.0f

/*
 * ============================================================
 * TIEMPOS
 * ============================================================
 */
#define TEMP_TASK_PERIOD_MS             500
#define CONFIG_TASK_PERIOD_MS           50
#define BUTTON_DEBOUNCE_MS              200

#endif