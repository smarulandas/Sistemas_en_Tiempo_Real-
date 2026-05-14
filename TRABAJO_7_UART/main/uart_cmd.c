#include "uart_cmd.h"
#include "board_config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "driver/uart.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "UART_CMD";

/*
 * ============================================================
 * FUNCIÓN PARA ESCRIBIR TEXTO POR UART
 * ============================================================
 *
 * Se usa para mostrar mensajes directamente en el monitor serial.
 */
static void uart_cmd_write_text(const char *text)
{
    uart_write_bytes(UART_COMMAND_PORT, text, strlen(text));
}

/*
 * ============================================================
 * CONVERTIR TEXTO A MAYÚSCULAS
 * ============================================================
 *
 * Esto permite que funcionen comandos escritos como:
 *
 * help
 * Help
 * HELP
 */
static void string_to_upper(char *text)
{
    for (int i = 0; text[i] != '\0'; i++) {
        text[i] = (char)toupper((unsigned char)text[i]);
    }
}

/*
 * ============================================================
 * REEMPLAZAR COMAS POR PUNTOS
 * ============================================================
 *
 * Permite que un usuario pueda escribir:
 *
 * LIM_MI_R 5,5
 *
 * y se convierta internamente en:
 *
 * LIM_MI_R 5.5
 */
static void replace_comma_with_dot(char *text)
{
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == ',') {
            text[i] = '.';
        }
    }
}

/*
 * ============================================================
 * LIMPIAR ESPACIOS AL INICIO Y AL FINAL
 * ============================================================
 *
 * Ejemplo:
 *
 * "   HELP   "  ->  "HELP"
 */
static void trim_whitespace(char *text)
{
    char *start = text;

    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    size_t len = strlen(text);

    while (len > 0 && isspace((unsigned char)text[len - 1])) {
        text[len - 1] = '\0';
        len--;
    }
}

/*
 * ============================================================
 * LIMPIAR CARACTERES NO IMPRIMIBLES
 * ============================================================
 *
 * A veces el monitor serial puede enviar caracteres raros.
 * Esta función deja solo caracteres normales.
 */
static void remove_non_printable_chars(char *text)
{
    size_t read_index = 0;
    size_t write_index = 0;

    while (text[read_index] != '\0') {
        unsigned char c = (unsigned char)text[read_index];

        if (c >= 32 && c <= 126) {
            text[write_index] = text[read_index];
            write_index++;
        }

        read_index++;
    }

    text[write_index] = '\0';
}

/*
 * ============================================================
 * PREPARAR LÍNEA RECIBIDA
 * ============================================================
 *
 * Hace todo el preprocesamiento del comando:
 *
 * - Quita espacios de sobra.
 * - Quita caracteres raros.
 * - Cambia coma decimal por punto.
 * - Convierte a mayúsculas.
 */
static void normalize_line(char *line)
{
    remove_non_printable_chars(line);
    trim_whitespace(line);
    replace_comma_with_dot(line);
    string_to_upper(line);
}

/*
 * ============================================================
 * PARSEAR COMANDO Y VALOR
 * ============================================================
 *
 * Acepta estos formatos:
 *
 * LIM_MI_R 5
 * LIM_MI_R=5
 * LIM_MI_R:5
 * LIM_MI_R_5
 *
 * INT_R 80
 * INT_R=80
 * INT_R_80
 *
 * También acepta comandos sin valor:
 *
 * HELP
 * INFO
 * COLOR
 */
static bool parse_command_and_value(
    char *line,
    char *cmd,
    size_t cmd_size,
    float *value,
    bool *has_value
)
{
    char temp_line[UART_COMMAND_BUFFER_SIZE] = {0};
    char first_token[64] = {0};
    float temp_value = 0.0f;

    *has_value = false;
    *value = 0.0f;

    strncpy(temp_line, line, sizeof(temp_line) - 1);
    temp_line[sizeof(temp_line) - 1] = '\0';

    /*
     * Permitir formatos:
     *
     * INT_R=50
     * INT_R:50
     *
     * convirtiéndolos a:
     *
     * INT_R 50
     */
    for (int i = 0; temp_line[i] != '\0'; i++) {
        if (temp_line[i] == '=' || temp_line[i] == ':') {
            temp_line[i] = ' ';
        }
    }

    /*
     * Primer intento:
     *
     * COMANDO VALOR
     */
    int items = sscanf(temp_line, "%63s %f", first_token, &temp_value);

    if (items == 2) {
        strncpy(cmd, first_token, cmd_size - 1);
        cmd[cmd_size - 1] = '\0';

        *value = temp_value;
        *has_value = true;

        return true;
    }

    /*
     * Segundo intento:
     *
     * Solo comando.
     *
     * Puede ser:
     * HELP
     * INFO
     * COLOR
     *
     * O puede ser:
     * INT_R_80
     * LIM_MI_R_5
     */
    if (items == 1) {
        char token_copy[64] = {0};

        strncpy(token_copy, first_token, sizeof(token_copy) - 1);
        token_copy[sizeof(token_copy) - 1] = '\0';

        /*
         * Buscar el último guion bajo.
         *
         * Ejemplo:
         * LIM_MI_R_5
         *
         * Último guion bajo antes del 5.
         */
        char *last_underscore = strrchr(token_copy, '_');

        if (last_underscore != NULL && *(last_underscore + 1) != '\0') {
            char *number_text = last_underscore + 1;
            char *end_ptr = NULL;

            temp_value = strtof(number_text, &end_ptr);

            /*
             * Si después del número no queda texto,
             * entonces sí era un valor válido.
             */
            if (end_ptr != number_text && *end_ptr == '\0') {
                *last_underscore = '\0';

                strncpy(cmd, token_copy, cmd_size - 1);
                cmd[cmd_size - 1] = '\0';

                *value = temp_value;
                *has_value = true;

                return true;
            }
        }

        /*
         * Si no tenía valor, se interpreta como comando simple:
         *
         * HELP
         * INFO
         * COLOR
         */
        strncpy(cmd, first_token, cmd_size - 1);
        cmd[cmd_size - 1] = '\0';

        return true;
    }

    return false;
}

/*
 * ============================================================
 * IDENTIFICAR TIPO DE COMANDO
 * ============================================================
 *
 * Convierte el texto del comando a un valor del enum uart_cmd_type_t.
 */
static uart_cmd_type_t get_command_type(const char *cmd)
{
    /*
     * Límites del color rojo.
     */
    if (strcmp(cmd, "LIM_MI_R") == 0) {
        return UART_CMD_SET_LIM_MI_R;
    }

    if (strcmp(cmd, "LIM_MA_R") == 0) {
        return UART_CMD_SET_LIM_MA_R;
    }

    /*
     * Límites del color azul.
     */
    if (strcmp(cmd, "LIM_MI_B") == 0) {
        return UART_CMD_SET_LIM_MI_B;
    }

    if (strcmp(cmd, "LIM_MA_B") == 0) {
        return UART_CMD_SET_LIM_MA_B;
    }

    /*
     * Límites del color verde.
     */
    if (strcmp(cmd, "LIM_MI_G") == 0) {
        return UART_CMD_SET_LIM_MI_G;
    }

    if (strcmp(cmd, "LIM_MA_G") == 0) {
        return UART_CMD_SET_LIM_MA_G;
    }

    /*
     * Intensidades del LED de temperatura.
     */
    if (strcmp(cmd, "INT_R") == 0 ||
        strcmp(cmd, "INTENSIDAD_R") == 0) {
        return UART_CMD_SET_INT_R;
    }

    if (strcmp(cmd, "INT_B") == 0 ||
        strcmp(cmd, "INTENSIDAD_B") == 0) {
        return UART_CMD_SET_INT_B;
    }

    if (strcmp(cmd, "INT_G") == 0 ||
        strcmp(cmd, "INTENSIDAD_G") == 0) {
        return UART_CMD_SET_INT_G;
    }

    /*
     * Comandos de consulta.
     */
    if (strcmp(cmd, "INFO") == 0 ||
        strcmp(cmd, "ESTADO") == 0 ||
        strcmp(cmd, "TEMPERATURA") == 0) {
        return UART_CMD_PRINT_INFO;
    }

    if (strcmp(cmd, "COLOR") == 0 ||
        strcmp(cmd, "POT") == 0 ||
        strcmp(cmd, "POTENCIOMETRO") == 0 ||
        strcmp(cmd, "VALORES_LED") == 0) {
        return UART_CMD_PRINT_COLOR;
    }

    if (strcmp(cmd, "HELP") == 0 ||
        strcmp(cmd, "AYUDA") == 0 ||
        strcmp(cmd, "?") == 0) {
        return UART_CMD_PRINT_HELP;
    }

    return UART_CMD_INVALID;
}

/*
 * ============================================================
 * PARSEAR LÍNEA COMPLETA
 * ============================================================
 *
 * Recibe una línea escrita por UART y devuelve un comando estructurado.
 */
static uart_command_t parse_uart_line(char *line)
{
    uart_command_t result = {
        .type = UART_CMD_INVALID,
        .value = 0.0f
    };

    char cmd[64] = {0};
    float value = 0.0f;
    bool has_value = false;

    normalize_line(line);

    if (strlen(line) == 0) {
        result.type = UART_CMD_NONE;
        return result;
    }

    if (!parse_command_and_value(line, cmd, sizeof(cmd), &value, &has_value)) {
        result.type = UART_CMD_INVALID;
        return result;
    }

    result.type = get_command_type(cmd);
    result.value = value;

    /*
     * Estos comandos obligatoriamente necesitan valor numérico.
     */
    switch (result.type) {
        case UART_CMD_SET_LIM_MI_R:
        case UART_CMD_SET_LIM_MA_R:
        case UART_CMD_SET_LIM_MI_B:
        case UART_CMD_SET_LIM_MA_B:
        case UART_CMD_SET_LIM_MI_G:
        case UART_CMD_SET_LIM_MA_G:
        case UART_CMD_SET_INT_R:
        case UART_CMD_SET_INT_B:
        case UART_CMD_SET_INT_G:
            if (!has_value) {
                result.type = UART_CMD_INVALID;
            }
            break;

        default:
            break;
    }

    return result;
}

/*
 * ============================================================
 * TAREA DE RECEPCIÓN UART
 * ============================================================
 *
 * Lee carácter por carácter.
 * Cuando detecta ENTER, procesa el comando.
 */
static void uart_rx_task(void *args)
{
    QueueHandle_t command_queue = (QueueHandle_t)args;

    uint8_t rx_byte = 0;

    char line_buffer[UART_COMMAND_BUFFER_SIZE] = {0};
    size_t line_index = 0;

    bool ignoring_escape_sequence = false;

    uart_cmd_write_text("\r\n");
    uart_cmd_write_text("========================================\r\n");
    uart_cmd_write_text(" UART listo. Escriba HELP y presione ENTER\r\n");
    uart_cmd_write_text("========================================\r\n\r\n");

    while (1) {
        int length = uart_read_bytes(
            UART_COMMAND_PORT,
            &rx_byte,
            1,
            pdMS_TO_TICKS(UART_TASK_PERIOD_MS)
        );

        if (length > 0) {

            /*
             * Ignorar secuencias de escape del terminal.
             * Esto evita problemas con flechas o caracteres especiales.
             */
            if (rx_byte == 0x1B) {
                ignoring_escape_sequence = true;
                continue;
            }

            if (ignoring_escape_sequence) {
                if ((rx_byte >= 'A' && rx_byte <= 'Z') ||
                    (rx_byte >= 'a' && rx_byte <= 'z') ||
                    rx_byte == '~') {
                    ignoring_escape_sequence = false;
                }

                continue;
            }

            /*
             * ENTER:
             * Puede llegar como '\r', '\n' o ambos.
             */
            if (rx_byte == '\n' || rx_byte == '\r') {
                uart_cmd_write_text("\r\n");

                if (line_index > 0) {
                    line_buffer[line_index] = '\0';

                    uart_command_t command = parse_uart_line(line_buffer);

                    /*
                     * Para depurar:
                     * Muestra el comando ya recibido antes de enviarlo a main.c.
                     */
                    uart_cmd_write_text("Comando recibido: ");
                    uart_cmd_write_text(line_buffer);
                    uart_cmd_write_text("\r\n");

                    if (command.type != UART_CMD_NONE) {
                        xQueueSend(command_queue, &command, pdMS_TO_TICKS(50));
                    }

                    line_index = 0;
                    memset(line_buffer, 0, sizeof(line_buffer));
                }
            }

            /*
             * BACKSPACE o DELETE.
             */
            else if (rx_byte == 8 || rx_byte == 127) {
                if (line_index > 0) {
                    line_index--;
                    line_buffer[line_index] = '\0';

                    /*
                     * Borra visualmente el carácter en el monitor.
                     */
                    uart_cmd_write_text("\b \b");
                }
            }

            /*
             * Caracteres normales.
             */
            else {
                /*
                 * Solo se aceptan caracteres imprimibles.
                 */
                if (rx_byte >= 32 && rx_byte <= 126) {

                    /*
                     * Eco:
                     * Muestra en el monitor lo que el usuario escribe.
                     */
                    uart_write_bytes(UART_COMMAND_PORT, (const char *)&rx_byte, 1);

                    if (line_index < sizeof(line_buffer) - 1) {
                        line_buffer[line_index] = (char)rx_byte;
                        line_index++;
                    }
                    else {
                        line_index = 0;
                        memset(line_buffer, 0, sizeof(line_buffer));

                        uart_cmd_write_text("\r\nError: comando demasiado largo.\r\n");
                    }
                }
            }
        }
    }
}

/*
 * ============================================================
 * INICIALIZACIÓN UART
 * ============================================================
 *
 * Configura UART0 a 115200 baudios y crea la tarea de recepción.
 */
esp_err_t uart_cmd_init(QueueHandle_t command_queue)
{
    if (command_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uart_config_t uart_config = {
        .baud_rate = UART_COMMAND_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    esp_err_t ret = uart_param_config(UART_COMMAND_PORT, &uart_config);
    if (ret != ESP_OK) {
        return ret;
    }

    /*
     * No cambiamos los pines físicos de UART.
     * Así se conserva la comunicación normal del monitor serial.
     */
    ret = uart_set_pin(
        UART_COMMAND_PORT,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE
    );

    if (ret != ESP_OK) {
        return ret;
    }

    /*
     * Instala el driver UART.
     *
     * Si ya estaba instalado, no se considera error grave.
     */
    ret = uart_driver_install(
        UART_COMMAND_PORT,
        UART_COMMAND_BUFFER_SIZE,
        UART_COMMAND_BUFFER_SIZE,
        0,
        NULL,
        0
    );

    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    /*
     * Crea la tarea que estará leyendo comandos.
     */
    BaseType_t task_created = xTaskCreate(
        uart_rx_task,
        "uart_rx_task",
        4096,
        command_queue,
        4,
        NULL
    );

    if (task_created != pdPASS) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "UART listo a %d baudios", UART_COMMAND_BAUD_RATE);

    return ESP_OK;
}