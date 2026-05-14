// Evita incluir múltiples veces el archivo de encabezado
#ifndef UART_CMD_H
// Define la constante para indicar que el archivo ya fue incluido
#define UART_CMD_H

// Incluye el archivo de errores de ESP32
#include "esp_err.h"
// Incluye la librería base de FreeRTOS
#include "freertos/FreeRTOS.h"
// Incluye la librería de colas de FreeRTOS
#include "freertos/queue.h"

// Define una enumeración con los tipos de comandos que pueden llegar por UART
typedef enum {
    // Comando nulo/sin comando
    UART_CMD_NONE = 0,

    // Comandos para establecer límite mínimo y máximo del canal Rojo
    UART_CMD_SET_LIM_MI_R,
    UART_CMD_SET_LIM_MA_R,

    // Comandos para establecer límite mínimo y máximo del canal Azul
    UART_CMD_SET_LIM_MI_B,
    UART_CMD_SET_LIM_MA_B,

    // Comandos para establecer límite mínimo y máximo del canal Verde
    UART_CMD_SET_LIM_MI_G,
    UART_CMD_SET_LIM_MA_G,

    // Comandos para establecer intensidad de canales Rojo, Azul y Verde
    UART_CMD_SET_INT_R,
    UART_CMD_SET_INT_B,
    UART_CMD_SET_INT_G,

    // Comandos para imprimir información, color e instrucciones de ayuda
    UART_CMD_PRINT_INFO,
    UART_CMD_PRINT_COLOR,
    UART_CMD_PRINT_HELP,

    // Comando inválido o no reconocido
    UART_CMD_INVALID
} uart_cmd_type_t;

// Define una estructura para almacenar un comando recibido por UART
typedef struct {
    // Campo que indica el tipo de comando recibido
    uart_cmd_type_t type;
    // Campo que almacena el valor numérico asociado al comando
    float value;
} uart_command_t;

// Declara la función de inicialización del módulo UART, recibe una cola de comandos
esp_err_t uart_cmd_init(QueueHandle_t command_queue);

// Finaliza las directivas condicionales de inclusión del archivo
#endif