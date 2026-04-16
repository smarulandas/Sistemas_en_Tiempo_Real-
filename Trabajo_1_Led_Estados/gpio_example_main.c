#include <stdio.h> //Permie usar funciones estandar de c
#include <stdbool.h> //Permite usar el tipo bool (true/false)
#include <stdint.h> //Permite usar tipos enteros como uint32_t (sin signo, sin neagtivo, entero, 32 bits, tipo)

#include "freertos/FreeRTOS.h" //Libreria base de FreeRTOS (Sistema Operativo (S.O) en tiempo real RTOS )
// Sirve para ejecutar varias tareas al mismo timempo, usar delays sin bloqeuar todo, usar colas, usar interrupciones

#include "freertos/task.h" //Permite crear tareas y usar vTaskDelay()
#include "freertos/queue.h" //Permite crear y usar colas

#include "driver/gpio.h" //Libreria para configurar y leer GPIO
#include "esp_err.h" //Manejo de errores de ESP-IDF
#include "esp_log.h" //Permite imprimir mensajes por monitor serial
#include "led_strip.h" //Librebria para controlar el LED RGB WS2812

#define LED_STRIP_RMT_RES_HZ   (10 * 1000 * 1000)   // Resolución del periférico RMT: 10 MHz
#define LED_GPIO              GPIO_NUM_8            // GPIO donde esta conectado el LED RGB de la placa o el PIN conecatdo
#define LED_NUM                 1                     // Solo hay 1 LED RGB en la placa

#define BOOT_BUTTON_GPIO       9                     // GPIO del botón BOOT
#define BUTTON_ACTIVE_LEVEL    0                     // El botón BOOT es activo en bajo: cuando se presiona vale 0

#define DEBOUNCE_MS            200                   // Tiempo de anti-rebote del botón en milisegundos, evitar que una sola pulsacion del boton se cuente varias veces.
#define CHECK_QUEUE_SLICE_MS   20                    // Tiempo pequeño para revisar cambios de estado sin bloquear demasiado
 
// Enumeración de los 5 estados del LED, por eso es enum
typedef enum {
    LED_STATE_1 = 0,                                 // Estado 1: ON 2 s / OFF 2 s
    LED_STATE_2,                                     // Estado 2: ON 1 s / OFF 1 s
    LED_STATE_3,                                     // Estado 3: ON 0.5 s / OFF 0.5 s
    LED_STATE_4,                                     // Estado 4: ON 0.1 s / OFF 0.1 s
    LED_STATE_5                                      // Estado 5: OFF fijo
} led_state_t;

// Al poner LED_STATE_1 = 0, c asigna los demas STATES como 1,2,3 ...
// El 0 indica que primero state es el 0, mas no que el state equivale a 0.

// Estructura para guardar los tiempos de cada estado. 

typedef struct {
    int on_time_ms;                                  // Tiempo encendido en milisegundos
    int off_time_ms;                                 // Tiempo apagado en milisegundos
} state_timing_t;

//             Función auxiliar que devuelve los tiempos ON/OFF según el estado actual

//static -> Solo es visible dentro de este archivo, no se puede usar desde otros archivos. 
// led_state_t -> Recibe el estado actual del LED para saber qué tiempos devolver
// state -> Nombre de la variable que entra
// get_state_timing -> Nombre de la función que devuelve los tiempos ON/OFF para cada estado

static state_timing_t get_state_timing(led_state_t state)
{
    switch (state) {                                 // Evalúa cuál es el estado actual
        case LED_STATE_1:                            // Si estamos en el estado 1
            return (state_timing_t){2000, 2000};     // Devuelve ON 2000 ms y OFF 2000 ms

        case LED_STATE_2:                            // Si estamos en el estado 2
            return (state_timing_t){1000, 1000};     // Devuelve ON 1000 ms y OFF 1000 ms

        case LED_STATE_3:                            // Si estamos en el estado 3
            return (state_timing_t){500, 500};       // Devuelve ON 500 ms y OFF 500 ms

        case LED_STATE_4:                            // Si estamos en el estado 4
            return (state_timing_t){100, 100};       // Devuelve ON 100 ms y OFF 100 ms

        case LED_STATE_5:                            // Si estamos en el estado 5
            return (state_timing_t){0, 0};           // Devuelve 0,0 porque en este estado se queda apagado

        default:                                     // Si por algún error llega un estado no válido
            return (state_timing_t){2000, 2000};     // Usa por defecto los tiempos del estado 1
    }
}

// Función para encender el LED en color verde

// static → la función solo se usa dentro de este archivo
// void -> No devuelve ningún valor, solo hace una acción (encender el LED)
// led_on_green-> Nombre de la función que enciende el LED en verde
// led_strip_handle_t strip → Parámetro que recibe la función. Es un "handle" o referencia al LED strip que se va a controlar que creó la librería, para poder enviarle comandos de encendido y apagado.
// strip -> Nombre de la variable que entra a la función, representa el LED strip que se va a controlar. 

static void led_on_green(led_strip_handle_t strip)
{
    led_strip_set_pixel(strip, 0, 0, 30, 0);         // LED 0 con color verde: R=0, G=30, B=0
    led_strip_refresh(strip);                        // Envía el nuevo color al LED físicamente
}

// Función para apagar el LED
static void led_off(led_strip_handle_t strip)
{
    led_strip_clear(strip);                          // Borra el color del LED, es decir, lo apaga, gracias a led_strip_clear
}

// ISR del botón BOOT
// Esta función se ejecuta automáticamente cuando el botón genera una interrupción

// IRAM_ATTR -> Atributo de ESP-IDF qque dice al compilador que esa funcion se debe de poner en la memoria IRAM (memoria de acceso rápido), debe de ejecutarse lo mas rapido posible
// void * arg -> La función recibe un puntero genérico como argumento, que en este casose usara para pasar la cola donde enviaremos el evento del botón
// arg -> Nombre de la variable tipo puntero generico que recibe algun dato.

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    // QueueHandle_t -> Es de la libreria por FreeRTOS, se usa para manejar colas
    // arg -> llego como un void*, pero en realidad dentro habia la cola gpio_evt_queue entonces se convierte en archivo correcto  QueueHandle_t
    QueueHandle_t queue = (QueueHandle_t)arg;        // Recupera la cola que se pasó como argumento

    //  uint32_t -> se cra variable entera sin signo de 32 bits para guardar el número del GPIO que disparó la interrupción
    uint32_t gpio_num = BOOT_BUTTON_GPIO;            // Guarda el número del GPIO que disparó la interrupción

    //la ISR detecta el evento y mete un dato en la cola, luego otra tarea lo recibe
    // & direccion de memeoria de esta variable (es la direccion donde esta guardada el 9(gpio_num))
    xQueueSendFromISR(queue, &gpio_num, NULL);       // Envía el número de GPIO a la cola desde la ISR
}



//Flujo de la interrupción: Presionar boton -> GPIO detecta cambio -> se dispara interrupción -> Se ejecuta ISR -> ISR envia dato a cola -> Tarea recibe dato -> cambia estado del LED

// Tarea encargada de leer los eventos del botón y cambiar de estado
static void button_task(void *pvParameters)
{
    QueueHandle_t *queues = (QueueHandle_t *)pvParameters;    // Recupera el arreglo de colas que recibió la tarea
    QueueHandle_t gpio_evt_queue = queues[0];                 // Cola donde llegan los eventos del botón desde la ISR
    QueueHandle_t state_queue = queues[1];                    // Cola donde esta tarea enviará el nuevo estado al LED

    led_state_t current_state = LED_STATE_1;                  // Estado inicial: 1
    TickType_t last_press_tick = 0;                           // Guarda el tiempo de la última pulsación válida

    while (1) {                                               // Bucle infinito de la tarea
        uint32_t io_num;                                      // Variable donde recibiremos el GPIO desde la cola

        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {   // Espera hasta que llegue un evento del botón
            TickType_t now = xTaskGetTickCount();                      // Guarda el tiempo actual del sistema

            if ((now - last_press_tick) * portTICK_PERIOD_MS > DEBOUNCE_MS) {  // Verifica anti-rebote
                last_press_tick = now;                                  // Actualiza el tiempo de la última pulsación válida

                current_state = (current_state + 1) % 5;                // Avanza al siguiente estado y vuelve a 0 después del 4
                xQueueOverwrite(state_queue, &current_state);           // Envía el nuevo estado a la tarea del LED
            }
        }
    }
}

// Función auxiliar para esperar cierto tiempo, pero revisando si llegó un cambio de estado
static bool wait_or_change_state(QueueHandle_t state_queue, led_state_t *state, int total_ms)
{
    int elapsed_ms = 0;                                      // Lleva la cuenta del tiempo ya esperado
    led_state_t new_state;                                   // Variable temporal por si llega un nuevo estado

    while (elapsed_ms < total_ms) {                          // Repite hasta completar el tiempo total
        int slice = CHECK_QUEUE_SLICE_MS;                    // Toma un pequeño bloque de tiempo de espera

        if ((total_ms - elapsed_ms) < slice) {               // Si ya falta menos de ese bloque
            slice = total_ms - elapsed_ms;                   // Ajusta el bloque final al tiempo que falta
        }

        if (xQueueReceive(state_queue, &new_state, pdMS_TO_TICKS(slice))) {  // Espera ese pequeño tiempo por un nuevo estado
            *state = new_state;                              // Si llega un nuevo estado, lo guarda
            return true;                                     // Indica que sí hubo cambio de estado
        }

        elapsed_ms += slice;                                 // Si no llegó nada, suma el bloque al tiempo esperado
    }

    return false;                                            // Si terminó el tiempo sin cambios, devuelve false
}

// Tarea encargada de hacer parpadear el LED según el estado actual
static void blink_task(void *pvParameters)
{
    QueueHandle_t state_queue = (QueueHandle_t)pvParameters; // Recupera la cola donde llegan los estados nuevos

    led_strip_handle_t strip;                                // Variable local que manejará el LED RGB

    led_strip_config_t strip_config = {                      // Estructura de configuración del LED strip
        .strip_gpio_num = LED_GPIO,                          // GPIO de datos del LED
        .max_leds = LED_NUM,                                 // Número de LEDs
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,            // Formato interno de color del WS2812
        .led_model = LED_MODEL_WS2812,                       // Modelo del LED RGB
        .flags.invert_out = false,                           // No invierte la señal
    };

    led_strip_rmt_config_t rmt_config = {                    // Estructura de configuración del periférico RMT
        .clk_src = RMT_CLK_SRC_DEFAULT,                      // Fuente de reloj por defecto
        .resolution_hz = LED_STRIP_RMT_RES_HZ,               // Resolución definida arriba
        .flags.with_dma = false,                             // No usa DMA
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &strip)); // Inicializa el LED strip

    led_state_t current_state = LED_STATE_1;                 // Estado inicial: 1
    led_state_t new_state;                                   // Variable temporal para recibir nuevos estados

    xQueueOverwrite(state_queue, &current_state);            // Mete el estado inicial en la cola

    while (1) {                                              // Bucle infinito de la tarea del LED

        if (xQueueReceive(state_queue, &new_state, 0)) {     // Revisa si llegó un cambio de estado sin bloquearse
            current_state = new_state;                       // Si llegó, actualiza el estado actual
        }

        if (current_state == LED_STATE_5) {                  // Si estamos en el estado 5
            led_off(strip);                                  // El LED debe estar apagado

            xQueueReceive(state_queue, &new_state, portMAX_DELAY); // Espera indefinidamente hasta que llegue un nuevo estado
            current_state = new_state;                       // Actualiza el estado cuando llegue
            continue;                                        // Vuelve a comenzar el while
        }

        state_timing_t timing = get_state_timing(current_state);    // Obtiene los tiempos ON/OFF del estado actual

        led_on_green(strip);                                 // Enciende el LED en verde

        if (wait_or_change_state(state_queue, &current_state, timing.on_time_ms)) { // Espera el tiempo ON o cambia de estado si llega evento
            continue;                                        // Si cambió de estado, vuelve al inicio del while
        }

        led_off(strip);                                      // Apaga el LED

        if (wait_or_change_state(state_queue, &current_state, timing.off_time_ms)) { // Espera el tiempo OFF o cambia de estado si llega evento
            continue;                                        // Si cambió de estado, vuelve al inicio del while
        }
    }
}

// Función principal del programa
void app_main(void)
{
    static const char *TAG = "led_states";                   // Etiqueta para mensajes del monitor serial

    QueueHandle_t gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));   // Cola para eventos del botón
    QueueHandle_t state_queue = xQueueCreate(1, sizeof(led_state_t));     // Cola para enviar el estado actual al LED

    if (gpio_evt_queue == NULL || state_queue == NULL) {     // Verifica si alguna cola no se pudo crear
        ESP_LOGE(TAG, "Error creando colas");                // Imprime mensaje de error
        return;                                              // Sale de app_main si algo falló
    }

    gpio_config_t io_conf = {                                // Estructura para configurar el botón BOOT
        .pin_bit_mask = 1ULL << BOOT_BUTTON_GPIO,            // Selecciona el GPIO9
        .mode = GPIO_MODE_INPUT,                             // Lo configura como entrada
        .pull_up_en = GPIO_PULLUP_ENABLE,                    // Activa pull-up interno
        .pull_down_en = GPIO_PULLDOWN_DISABLE,               // Desactiva pull-down
        .intr_type = GPIO_INTR_NEGEDGE                       // Interrupción por flanco de bajada (cuando se presiona)
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));                  // Aplica la configuración al GPIO del botón
    ESP_ERROR_CHECK(gpio_install_isr_service(0));            // Instala el servicio de interrupciones GPIO
    ESP_ERROR_CHECK(gpio_isr_handler_add(BOOT_BUTTON_GPIO, gpio_isr_handler, (void *)gpio_evt_queue)); // Asocia la ISR al botón y le pasa la cola

    QueueHandle_t button_task_queues[2] = {gpio_evt_queue, state_queue}; // Arreglo local para pasar ambas colas a la tarea del botón

    xTaskCreate(button_task, "button_task", 4096, button_task_queues, 10, NULL); // Crea la tarea que procesa el botón
    xTaskCreate(blink_task, "blink_task", 4096, (void *)state_queue, 10, NULL);   // Crea la tarea que controla el LED
}