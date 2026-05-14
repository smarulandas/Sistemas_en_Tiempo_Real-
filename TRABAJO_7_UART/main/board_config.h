// Guardias de compilación para evitar inclusiones múltiples
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Incluye drivers para GPIO
#include "driver/gpio.h"
// Incluye tipos de datos para ADC
#include "hal/adc_types.h"
// Incluye driver para comunicación UART
#include "driver/uart.h"

/*
 * ============================================================
 * TIPO DE LED RGB
 * ============================================================
 *
 * LED RGB son de ánodo común:
 * - La pata más larga va a 3.3 V.
 * - Los colores se encienden cuando el GPIO baja.
 */
// Configura el LED RGB como ánodo común (activo en bajo)
#define RGB_COMMON_ANODE true

/*
 * ============================================================
 * LED RGB CONFIGURABLE - POTENCIÓMETRO Y BOTÓN
 * ============================================================
 */
// GPIO para el color rojo del LED configurable
#define LED_CONFIG_R_GPIO      GPIO_NUM_4
// GPIO para el color verde del LED configurable
#define LED_CONFIG_G_GPIO      GPIO_NUM_5
// GPIO para el color azul del LED configurable
#define LED_CONFIG_B_GPIO      GPIO_NUM_6

/*
 * ============================================================
 * LED RGB DE TEMPERATURA - NTC
 * ============================================================
 */
// GPIO para el color rojo del LED de temperatura
#define LED_TEMP_R_GPIO        GPIO_NUM_10
// GPIO para el color verde del LED de temperatura
#define LED_TEMP_G_GPIO        GPIO_NUM_18
// GPIO para el color azul del LED de temperatura
#define LED_TEMP_B_GPIO        GPIO_NUM_19

/*
 * ============================================================
 * BOTÓN
 * ============================================================
 *
 * Conexión:
 * GPIO7 ---- botón ---- GND
 */
// GPIO del botón de entrada (activo en bajo)
#define BUTTON_GPIO            GPIO_NUM_7

/*
 * ============================================================
 * ADC
 * ============================================================
 *
 * POT_ADC_CHANNEL:
 * - Potenciómetro.
 *
 * NTC_ADC_CHANNEL:
 * - Punto medio del divisor del NTC.
 */
// Canal ADC para leer el potenciómetro
#define POT_ADC_CHANNEL        ADC_CHANNEL_0
// Canal ADC para leer el sensor NTC
#define NTC_ADC_CHANNEL        ADC_CHANNEL_1

/*
 * ============================================================
 * CALIBRACIÓN DEL POTENCIÓMETRO
 * ============================================================
 *
 * El ADC ideal va de 0 a 4095.
 *
 * En este montaje el potenciómetro al máximo está llegando
 * aproximadamente al 72%, es decir cerca de 2950.
 *
 * Por eso se toma:
 * 0    -> 0%
 * 2950 -> 100%
 */
// Valor mínimo del ADC del potenciómetro
#define POT_ADC_MIN_RAW                 0
// Valor máximo del ADC del potenciómetro (calibrado al 72%)
#define POT_ADC_MAX_RAW                 2950

/*
 * ============================================================
 * CONFIGURACIÓN DEL NTC
 * ============================================================
 *
 * 
 * - NTC de 10 kΩ.
 * - Resistencia fija de 10 kΩ.
 *
 * Conexión:
 *
 * 3.3V ---- R fija 10k ---- GPIO ADC ---- NTC 10k ---- GND
 */
// Resistencia nominal del NTC a temperatura de referencia
#define NTC_NOMINAL_RESISTANCE_OHMS     10000.0f
// Resistencia fija en serie con el NTC
#define NTC_FIXED_RESISTOR_OHMS         10000.0f
// Coeficiente beta del NTC (característica térmica)
#define NTC_BETA_VALUE                  3950.0f 
// Temperatura nominal de referencia del NTC en Kelvin
#define NTC_NOMINAL_TEMPERATURE_K       298.15f

// Voltaje de referencia del ADC en milivoltios
#define ADC_REFERENCE_MV                3300.0f

/*
 * Corrección de calibración de temperatura.
 *
 * Si el sistema marca 31 °C y la temperatura real es 22 °C:
 *
 * offset = temperatura_real - temperatura_medida
 * offset = 22 - 31 = -9
 */

#define TEMP_CALIBRATION_OFFSET_C       (-3.5f)
#define TEMP_CALIBRATION_GAIN           1.0f


/*
 * ============================================================
 * RANGOS INICIALES DEL LED DE TEMPERATURA
 * ============================================================
 *
 * Estos son los valores iniciales:
 *
 * Red   entre 5 y 20 °C.
 * Blue  entre 10 y 30 °C.
 * Green entre 15 y 40 °C.
 *
 * Como los rangos se cruzan:
 * - Si T = 5, prende Red.
 * - Si T = 10, prende Red y Blue.
 * - Si T = 17, prenden Red, Blue y Green.
 */
// Temperatura mínima para encender rojo
#define DEFAULT_LIM_MI_R_C              5.0f
// Temperatura máxima para encender rojo
#define DEFAULT_LIM_MA_R_C              20.0f

// Temperatura mínima para encender azul
#define DEFAULT_LIM_MI_B_C              10.0f
// Temperatura máxima para encender azul
#define DEFAULT_LIM_MA_B_C              30.0f

// Temperatura mínima para encender verde
#define DEFAULT_LIM_MI_G_C              15.0f
// Temperatura máxima para encender verde
#define DEFAULT_LIM_MA_G_C              40.0f

/*
 * Intensidad inicial de cada color del LED de temperatura.
 * Va de 0 a 100%.
 */
// Intensidad inicial del LED rojo de temperatura (100%)
#define DEFAULT_INT_R_PERCENT           100
// Intensidad inicial del LED azul de temperatura (100%)
#define DEFAULT_INT_B_PERCENT           100
// Intensidad inicial del LED verde de temperatura (100%)
#define DEFAULT_INT_G_PERCENT           100

/*
 * ============================================================
 * UART
 * ============================================================
 *
 * Se usa UART0 a 115200 baudios para recibir comandos.
 *
 * Nota:
 * Si tu monitor serial ya usa UART0, puedes escribir los comandos desde
 * el monitor serial de ESP-IDF.
 */
// Puerto UART para recibir comandos desde el monitor serial
#define UART_COMMAND_PORT               UART_NUM_0
// Velocidad de transmisión UART
#define UART_COMMAND_BAUD_RATE          115200
// Tamaño del buffer de recepción UART
#define UART_COMMAND_BUFFER_SIZE        256

/*
 * ============================================================
 * TIEMPOS DE TAREAS
 * ============================================================
 */
// Período de actualización del LED de temperatura (lectura del NTC)
#define TEMP_TASK_PERIOD_MS             2000 //Tiempo entre lecturas del NTC y actualización del LED de temperatura
// Período de actualización del LED configurable
#define CONFIG_TASK_PERIOD_MS           50
// Período de procesamiento de comandos UART
#define UART_TASK_PERIOD_MS             50
// Tiempo de debounce del botón para evitar lecturas erráticas
#define BUTTON_DEBOUNCE_MS              200

// Cierre de guardias de compilación
#endif