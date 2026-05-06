#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "driver/gpio.h"      // Permite usar nombres de pines como GPIO_NUM_4.
#include "hal/adc_types.h"    // Permite usar canales ADC como ADC_CHANNEL_0.

/*
 * ============================================================
 * TIPO DE LED RGB
 * ============================================================
 *
 * En este proyecto los LED RGB son de ÁNODO COMÚN.
 *
 * Ánodo común significa:
 * - La pata más larga del LED RGB va a 3.3 V.
 * - Cada color se enciende cuando el GPIO entrega nivel bajo.
 *
 * Por eso el código debe invertir el PWM internamente.
 */
#define RGB_COMMON_ANODE true

/*
 * ============================================================
 * LED RGB CONFIGURABLE - PUNTO 2
 * ============================================================
 *
 * Este LED RGB se controla con:
 * - Potenciómetro: ajusta la intensidad.
 * - Botón: cambia de estado.
 *
 * Estados:
 * CONFIG_RED   -> controla rojo.
 * CONFIG_BLUE  -> controla azul.
 * CONFIG_GREEN -> controla verde.
 * SHOW_COLOR   -> muestra la mezcla guardada.
 */
#define LED_CONFIG_R_GPIO      GPIO_NUM_4   // Pin para el color rojo del LED configurable.
#define LED_CONFIG_G_GPIO      GPIO_NUM_5   // Pin para el color verde del LED configurable.
#define LED_CONFIG_B_GPIO      GPIO_NUM_6   // Pin para el color azul del LED configurable.

/*
 * ============================================================
 * LED RGB DE TEMPERATURA - PUNTO 1
 * ============================================================
 *
 * Este LED RGB indica la temperatura medida por el NTC:
 * - Azul  si T < 25 °C.
 * - Verde si 25 °C <= T <= 35 °C.
 * - Rojo  si T > 35 °C.
 */
#define LED_TEMP_R_GPIO        GPIO_NUM_10  // Pin para el color rojo del LED de temperatura.
#define LED_TEMP_G_GPIO        GPIO_NUM_18  // Pin para el color verde del LED de temperatura.
#define LED_TEMP_B_GPIO        GPIO_NUM_19  // Pin para el color azul del LED de temperatura.

/*
 * ============================================================
 * BOTÓN
 * ============================================================
 *
 * Conexión:
 * GPIO7 ---- botón ---- GND
 *
 * El código activa la resistencia pull-up interna.
 * Por eso:
 * - Sin presionar: el pin lee 1.
 * - Presionado: el pin lee 0.
 */
#define BUTTON_GPIO            GPIO_NUM_7

/*
 * ============================================================
 * CANALES ADC
 * ============================================================
 *
 * POT_ADC_CHANNEL:
 * - Lee el voltaje del pin central del potenciómetro.
 *
 * NTC_ADC_CHANNEL:
 * - Lee el voltaje del punto medio del divisor de tensión del NTC.
 *
 * En el montaje usado:
 * - ADC_CHANNEL_0 se usa para el potenciómetro.
 * - ADC_CHANNEL_1 se usa para el NTC.
 */
#define POT_ADC_CHANNEL        ADC_CHANNEL_0
#define NTC_ADC_CHANNEL        ADC_CHANNEL_1

/*
 * ============================================================
 * CONFIGURACIÓN DEL NTC
 * ============================================================
 *
 * Se usó:
 * - NTC de 10 kΩ.
 * - Resistencia fija de 10 kΩ.
 *
 * El NTC de 10 kΩ significa:
 * - Aproximadamente a 25 °C el NTC mide 10 kΩ.
 *
 * La resistencia fija también se toma de 10 kΩ porque así el divisor
 * queda aproximadamente en la mitad de 3.3 V cuando la temperatura está
 * cerca de 25 °C. Esto mejora la sensibilidad cerca de la temperatura
 * ambiente.
 *
 * Conexión usada:
 *
 * 3.3V ---- Resistencia fija 10k ---- GPIO ADC ---- NTC 10k ---- GND
 */
#define NTC_NOMINAL_RESISTANCE_OHMS     10000.0f  // Valor nominal del NTC a 25 °C.
#define NTC_FIXED_RESISTOR_OHMS         10000.0f  // Valor real de la resistencia fija del divisor.
#define NTC_BETA_VALUE                  3950.0f   // Valor Beta aproximado del NTC.
#define NTC_NOMINAL_TEMPERATURE_K       298.15f   // 25 °C expresados en Kelvin.

/*
 * Voltaje de referencia usado para convertir el valor ADC a milivoltios.
 * Para una práctica funciona bien usar 3300 mV.
 */
#define ADC_REFERENCE_MV                3300.0f

/*
 * ============================================================
 * UMBRALES DE TEMPERATURA
 * ============================================================
 */
#define TEMP_BLUE_LIMIT_C               25.0f     // Menor que este valor: azul.
#define TEMP_RED_LIMIT_C                35.0f     // Mayor que este valor: rojo.

/*
 * ============================================================
 * TIEMPOS DE LAS TAREAS
 * ============================================================
 *
 * TEMP_TASK_PERIOD_MS:
 * - Cada cuánto se lee el NTC.
 *
 * CONFIG_TASK_PERIOD_MS:
 * - Cada cuánto se lee el potenciómetro.
 *
 * BUTTON_DEBOUNCE_MS:
 * - Tiempo de antirrebote del botón.
 */
#define TEMP_TASK_PERIOD_MS             500       // Lee temperatura cada 500 ms.
#define CONFIG_TASK_PERIOD_MS           50        // Lee potenciómetro cada 50 ms.
#define BUTTON_DEBOUNCE_MS              200       // Evita múltiples cambios por rebote.

#endif
