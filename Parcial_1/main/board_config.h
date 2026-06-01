#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "hal/adc_types.h"
#include "driver/ledc.h"

/*
 * ============================================================
 * CONFIGURACIÓN GENERAL DEL PROYECTO
 * ============================================================
 *
 * Aquí van todos los GPIO, canales ADC, canales PWM,
 * rangos e intensidades.
 *
 * Si cambias un pin físico, solo lo cambias aquí.
 */

/*
 * ============================================================
 * ADC: POTENCIÓMETRO Y NTC
 * ============================================================
 *
 * IMPORTANTE:
 * Cambia estos canales si tu potenciómetro o NTC están conectados
 * a otros canales ADC.
 */
#define POT_ADC_CHANNEL     ADC_CHANNEL_0
#define NTC_ADC_CHANNEL     ADC_CHANNEL_1

/*
 * Voltaje de referencia usado para convertir ADC a voltaje.
 */
#define ADC_REFERENCE_MV    3300.0f

/*
 * Rango del ADC para el potenciómetro.
 */
#define POT_ADC_MIN_RAW     0
#define POT_ADC_MAX_RAW     4095

/*
 * ============================================================
 * PARÁMETROS DEL SENSOR NTC
 * ============================================================
 *
 * Valores típicos para un NTC de 10k.
 */
#define NTC_FIXED_RESISTOR_OHMS       10000.0f
#define NTC_NOMINAL_RESISTANCE_OHMS   10000.0f
#define NTC_NOMINAL_TEMPERATURE_K     298.15f
#define NTC_BETA_VALUE                3950.0f

/*
 * Calibración opcional de temperatura.
 */
#define TEMP_CALIBRATION_GAIN         1.0f
#define TEMP_CALIBRATION_OFFSET_C     (-3.5f)

/*
 * ============================================================
 * BOTÓN PARA CAMBIAR UNIDAD
 * ============================================================
 *
 * Conexión recomendada:
 *
 * Un lado del botón -> GPIO
 * Otro lado         -> GND
 *
 * Así se usa pull-up interno.
 */
#define BUTTON_UNIT_GPIO      9
#define BUTTON_ACTIVE_LEVEL   0

/*
 * ============================================================
 * RGB1: RANGOS DE TEMPERATURA
 * ============================================================
 *
 * RGB1 se usa para:
 *
 * Rojo  entre a y b °C.
 * Verde entre c y d °C.
 * Azul  entre e y f °C.
 */
#define RGB1_RED_GPIO      4
#define RGB1_GREEN_GPIO    5
#define RGB1_BLUE_GPIO     6

/*
 * Canales PWM del RGB1.
 */
#define RGB1_RED_LEDC_CHANNEL      LEDC_CHANNEL_0
#define RGB1_GREEN_LEDC_CHANNEL    LEDC_CHANNEL_1
#define RGB1_BLUE_LEDC_CHANNEL     LEDC_CHANNEL_2

/*
 * ============================================================
 * RGB2: LED ROJO DEL UMBRAL
 * ============================================================
 *
 * RGB2 se usa únicamente para el punto 4.
 *
 * Solo se prende el canal rojo cuando:
 *
 * temperatura_actual >= umbral_del_potenciómetro
 */
#define RGB2_RED_GPIO      7
#define RGB2_GREEN_GPIO    8
#define RGB2_BLUE_GPIO     10

/*
 * Canales PWM del RGB2.
 */
#define RGB2_RED_LEDC_CHANNEL      LEDC_CHANNEL_3
#define RGB2_GREEN_LEDC_CHANNEL    LEDC_CHANNEL_4
#define RGB2_BLUE_LEDC_CHANNEL     LEDC_CHANNEL_5

/*
 * ============================================================
 * CONFIGURACION DE LED RGB
 * ============================================================
 *
 * RGB_ACTIVE_HIGH = 1:
 * - Catodo comun.
 * - La pata comun va a GND.
 * - El color prende con nivel alto.
 *
 * RGB_ACTIVE_HIGH = 0:
 * - Anodo comun.
 * - La pata comun va a 3.3 V.
 * - El color prende con nivel bajo.
 */
#define RGB_ACTIVE_HIGH    0

/*
 * Frecuencia PWM de ambos RGB.
 */
#define RGB_PWM_FREQ_HZ    5000

/*
 * ============================================================
 * INTENSIDADES INICIALES
 * ============================================================
 *
 * 0   = apagado
 * 255 = máximo brillo
 */
#define RGB1_RED_INTENSITY      255
#define RGB1_GREEN_INTENSITY    255
#define RGB1_BLUE_INTENSITY     255

#define RGB2_RED_INTENSITY      255

/*
 * ============================================================
 * RANGOS INICIALES DE RGB1
 * ============================================================
 *
 * Estos rangos también se podrán cambiar por comandos.
 */
#define RGB1_RED_MIN_C      0.0f
#define RGB1_RED_MAX_C      25.0f

#define RGB1_GREEN_MIN_C    25.1f
#define RGB1_GREEN_MAX_C    35.0f

#define RGB1_BLUE_MIN_C     35.1f
#define RGB1_BLUE_MAX_C     100.0f

/*
 * ============================================================
 * IMPRESIÓN DE TEMPERATURA
 * ============================================================
 */
#define TEMP_PRINT_PERIOD_DEFAULT_S   2U
#define TEMP_PRINT_PERIOD_MIN_S       1U
#define TEMP_PRINT_PERIOD_MAX_S       3600U

#endif