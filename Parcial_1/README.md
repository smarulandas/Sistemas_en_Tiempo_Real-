# Trabajo 7 - ESP32-C6: NTC, Potenciómetro y LED RGB

Este proyecto fue realizado para la materia **Sistemas en Tiempo Real** usando un **ESP32-C6**.  
El sistema integra un sensor de temperatura **NTC**, un **potenciómetro**, un **botón**, comunicación por **UART** y dos **LED RGB de ánodo común**.

El objetivo principal fue controlar dos LED RGB:

1. Un LED RGB que cambia sus colores según la temperatura medida por un NTC.
2. Un LED RGB configurable mediante potenciómetro y botón, guardando la intensidad de cada color.

---

## Requisitos 

### 1. LED RGB controlado por temperatura

Se debía usar un **NTC** para medir temperatura y encender los colores del LED RGB según rangos configurables.

Los rangos iniciales solicitados fueron:

| Color | Rango de temperatura |
|---|---|
| Red | 5 °C a 20 °C |
| Blue | 10 °C a 30 °C |
| Green | 15 °C a 40 °C |

Como los rangos se cruzan, pueden encenderse varios colores al mismo tiempo.

Ejemplos:

| Temperatura | Resultado |
|---:|---|
| 5 °C | Red |
| 10 °C | Red + Blue |
| 17 °C | Red + Blue + Green |
| 24 °C | Blue + Green |
| 35 °C | Green |

También se pidió poder configurar por UART:

- Límite mínimo de cada color.
- Límite máximo de cada color.
- Intensidad de cada color de 0% a 100%.

---

### 2. LED RGB configurable con potenciómetro y botón

El segundo LED RGB funciona mediante una máquina de estados.

Cada vez que se presiona el botón, el sistema cambia de estado:

1. `CONFIG_RED`: el potenciómetro controla la intensidad roja.
2. `CONFIG_BLUE`: el potenciómetro controla la intensidad azul.
3. `CONFIG_GREEN`: el potenciómetro controla la intensidad verde.
4. `SHOW_COLOR`: se muestra la mezcla final de los tres valores guardados.

Al cambiar de estado, el valor configurado queda guardado.

Además, el monitor serial muestra el porcentaje de intensidad configurado para cada color.

---

## Componentes utilizados

- ESP32-C6
- 1 NTC de 10 kΩ
- 1 resistencia fija de 10 kΩ para el divisor del NTC
- 1 potenciómetro de 10 kΩ
- 1 botón pulsador
- 2 LED RGB de ánodo común
- Resistencias para los LED RGB
- Protoboard
- Cables

---

## Conexiones principales

| Elemento | Pin usado |
|---|---|
| Potenciómetro | GPIO0 |
| NTC | GPIO1 |
| Botón | GPIO7 |
| LED RGB configurable - Rojo | GPIO4 |
| LED RGB configurable - Verde | GPIO5 |
| LED RGB configurable - Azul | GPIO6 |
| LED RGB temperatura - Rojo | GPIO10 |
| LED RGB temperatura - Verde | GPIO18 |
| LED RGB temperatura - Azul | GPIO19 |

Los LED RGB usados son de **ánodo común**, por lo tanto la pata más larga va conectada a **3.3 V**.

En el código esto se configuró así:

```c
#define RGB_COMMON_ANODE true
```

---

## Conexión del NTC

El NTC se usó en un divisor de tensión junto con una resistencia fija de 10 kΩ.

La conexión usada fue:

```text
3.3V ---- Resistencia fija 10k ---- GPIO1 ---- NTC 10k ---- GND
```

El ESP32-C6 lee el voltaje del punto medio mediante el ADC.

---

## Cálculo de temperatura

El ESP32-C6 no mide resistencia directamente.  
Primero mide el voltaje del divisor del NTC mediante el ADC.

Luego se calcula la resistencia del NTC usando la fórmula del divisor de tensión:

```text
R_NTC = R_FIJA × V_ADC / (VCC - V_ADC)
```

Después se usa la ecuación Beta del NTC:

```text
1/T = 1/T0 + (1/B) × ln(R/R0)
```

Donde:

| Variable | Significado |
|---|---|
| T | Temperatura en Kelvin |
| T0 | Temperatura nominal, 298.15 K |
| B | Valor Beta del NTC |
| R | Resistencia calculada del NTC |
| R0 | Resistencia nominal del NTC, 10 kΩ |

Finalmente, la temperatura se convierte a grados Celsius:

```text
°C = K - 273.15
```

---

## Configuración del NTC en el código

En `board_config.h` se configuró:

```c
#define NTC_NOMINAL_RESISTANCE_OHMS     10000.0f
#define NTC_FIXED_RESISTOR_OHMS         10000.0f
#define NTC_BETA_VALUE                  3950.0f
#define NTC_NOMINAL_TEMPERATURE_K       298.15f
#define ADC_REFERENCE_MV                3300.0f
```

También se agregó la visualización del voltaje leído en el punto del NTC, para comparar el valor del ADC con el valor medido físicamente con multímetro.

---

## Comandos UART implementados

El sistema permite escribir comandos desde el monitor serial.

### Comandos para límites de temperatura

| Comando | Función |
|---|---|
| `LIM_MI_R valor` | Cambia el límite mínimo del rojo |
| `LIM_MA_R valor` | Cambia el límite máximo del rojo |
| `LIM_MI_B valor` | Cambia el límite mínimo del azul |
| `LIM_MA_B valor` | Cambia el límite máximo del azul |
| `LIM_MI_G valor` | Cambia el límite mínimo del verde |
| `LIM_MA_G valor` | Cambia el límite máximo del verde |

Ejemplo:

```text
LIM_MI_R 5
LIM_MA_R 20
```

Eso significa que el rojo se encenderá entre 5 °C y 20 °C.

---

### Comandos para intensidad del LED de temperatura

| Comando | Función |
|---|---|
| `INT_R valor` | Cambia la intensidad del rojo de 0% a 100% |
| `INT_B valor` | Cambia la intensidad del azul de 0% a 100% |
| `INT_G valor` | Cambia la intensidad del verde de 0% a 100% |

Ejemplo:

```text
INT_B 50
```

Eso significa que el azul se encenderá al 50% cuando la temperatura esté dentro del rango configurado para Blue.

---

### Comandos de consulta

| Comando | Función |
|---|---|
| `HELP` | Muestra la lista de comandos disponibles |
| `INFO` | Muestra temperatura, voltaje, límites e intensidades |
| `COLOR` | Muestra los valores guardados del LED controlado por potenciómetro |

---

## Funcionamiento del potenciómetro

El potenciómetro se conecta como divisor de tensión:

```text
3.3V ---- Potenciómetro ---- GND
              |
            GPIO0
```

El pin central del potenciómetro entrega un voltaje variable que el ADC convierte a un valor numérico.

Ese valor se escala a un porcentaje de intensidad y luego a PWM:

```text
0%   → LED apagado
100% → LED con brillo máximo
```

El valor configurado se guarda según el estado actual:

| Estado | Color configurado |
|---|---|
| CONFIG_RED | Rojo |
| CONFIG_BLUE | Azul |
| CONFIG_GREEN | Verde |
| SHOW_COLOR | Mezcla final |

---

## Manejo del botón

El botón se conecta así:

```text
GPIO7 ---- Botón ---- GND
```

Se usa la resistencia interna pull-up del ESP32-C6.

El botón genera una interrupción, pero la interrupción no cambia directamente el estado.  
En su lugar, envía un evento a una cola de FreeRTOS.  
Luego una tarea procesa ese evento, aplica antirrebote y cambia al siguiente estado.

Esto permite un diseño más ordenado y evita hacer mucha lógica dentro de la interrupción.

---

## Tareas de FreeRTOS

El proyecto usa varias tareas:

| Tarea | Función |
|---|---|
| `temperature_task` | Lee el NTC, calcula temperatura y actualiza el LED RGB de temperatura |
| `color_config_task` | Lee el potenciómetro, procesa el botón y actualiza el LED RGB configurable |
| `uart_command_task` | Procesa los comandos recibidos por UART |
| `uart_rx_task` | Lee los caracteres escritos en el monitor serial |

---

## Estructura del proyecto

```text
TRABAJO_6_NTC_Y_POTENCIOMETRO/
│
├── README.md
├── CMakeLists.txt
├── sdkconfig
│
└── main/
    ├── main.c
    ├── board_config.h
    ├── analog.c
    ├── analog.h
    ├── button.c
    ├── button.h
    ├── led_rgb.c
    ├── led_rgb.h
    ├── uart_cmd.c
    ├── uart_cmd.h
    └── CMakeLists.txt
```

---

## Descripción de archivos principales

| Archivo | Función |
|---|---|
| `main.c` | Coordina las tareas, estados, comandos UART y lógica principal |
| `board_config.h` | Contiene pines, constantes del NTC, rangos iniciales e intensidades |
| `analog.c/h` | Lee el potenciómetro y el NTC mediante ADC |
| `led_rgb.c/h` | Controla los LED RGB usando PWM |
| `button.c/h` | Configura el botón con interrupción |
| `uart_cmd.c/h` | Recibe y procesa comandos desde el monitor serial |

---

## Compilación y carga

Desde la carpeta del proyecto:

```bash
idf.py set-target esp32c6
idf.py build
idf.py flash monitor
```

---

## Resultado final

El proyecto permite:

- Medir temperatura con un NTC.
- Mostrar la temperatura y el voltaje del NTC en el monitor serial.
- Encender uno o varios colores del LED RGB según rangos configurables.
- Cambiar límites de temperatura por UART.
- Cambiar intensidades de los colores por UART.
- Configurar otro LED RGB usando potenciómetro y botón.
- Guardar intensidades de rojo, azul y verde.
- Mostrar la mezcla final del LED configurado.
- Mostrar información del sistema en el monitor serial.

---

## Nota sobre calibración

Para mejorar la lectura del NTC se comparó el voltaje calculado por el ADC con el voltaje medido físicamente usando un multímetro.  
El cálculo de temperatura se realiza con la ecuación Beta del NTC, y los parámetros principales de calibración se encuentran en `board_config.h`.

La validación con multímetro permitió comprobar si la lectura del ADC era cercana al voltaje real del divisor.