# TRABAJO_6_NTC_Y_POTENCIOMETRO

Proyecto desarrollado con **ESP32-C6** usando **ESP-IDF**.  
El sistema utiliza un **NTC**, un **potenciómetro**, un **botón** y dos **LED RGB de ánodo común**.

## Objetivo general

Implementar un sistema con dos funciones principales:

1. Encender un LED RGB según la temperatura medida por un NTC.
2. Configurar el color de otro LED RGB usando un potenciómetro y un botón con máquina de estados.

## Funcionamiento del proyecto

### Punto 1: LED RGB por temperatura

El primer LED RGB cambia de color según la temperatura calculada con el NTC:

| Rango de temperatura | Color |
|---|---|
| Temperatura menor a 25 °C | Azul |
| Temperatura entre 25 °C y 35 °C | Verde |
| Temperatura mayor a 35 °C | Rojo |

El NTC se conecta en un divisor de tensión con una resistencia fija de **10 kΩ**.  
El ESP32-C6 lee el voltaje del divisor mediante ADC y calcula la temperatura usando la ecuación Beta del NTC.

La conexión usada es:
3.3V ---- Resistencia 10k ---- GPIO1 ---- NTC 10k ---- GND

### Punto 2: LED RGB configurable

El segundo LED RGB se controla con un potenciómetro y un botón.

Cada vez que se presiona el botón, el sistema cambia de estado:

1. `CONFIG_RED`: el potenciómetro controla la intensidad roja.
2. `CONFIG_BLUE`: el potenciómetro controla la intensidad azul.
3. `CONFIG_GREEN`: el potenciómetro controla la intensidad verde.
4. `SHOW_COLOR`: se muestra la mezcla final de los tres valores guardados.

Al cambiar de estado, se conserva el valor de intensidad del color configurado.

## Hardware utilizado

- ESP32-C6
- 2 LED RGB de ánodo común
- 1 NTC de 10 kΩ
- 1 resistencia fija de 10 kΩ para el divisor del NTC
- 1 potenciómetro de 10 kΩ
- 1 botón pulsador
- Resistencias para limitar corriente en los LED RGB
- Protoboard y cables

## Configuración importante

En `main/board_config.h` se configuró:

```c
#define RGB_COMMON_ANODE true

#define NTC_NOMINAL_RESISTANCE_OHMS     10000.0f
#define NTC_FIXED_RESISTOR_OHMS         10000.0f
```

Esto indica que los LED RGB son de **ánodo común** y que el NTC usado es de **10 kΩ** con una resistencia fija de **10 kΩ**.

## Conexiones principales

| Elemento | Pin ESP32-C6 |
|---|---|
| Potenciómetro | GPIO0 |
| NTC | GPIO1 |
| Botón | GPIO7 |
| RGB configurable - Rojo | GPIO4 |
| RGB configurable - Verde | GPIO5 |
| RGB configurable - Azul | GPIO6 |
| RGB temperatura - Rojo | GPIO10 |
| RGB temperatura - Verde | GPIO18 |
| RGB temperatura - Azul | GPIO19 |

## Compilación y carga

Desde la terminal de ESP-IDF:

```bash
idf.py set-target esp32c6
idf.py build
idf.py flash monitor
```

## Estructura del proyecto

```text
TRABAJO_6_NTC_Y_POTENCIOMETRO/
│
├── CMakeLists.txt
├── README.md
├── .gitignore
│
└── main/
    ├── CMakeLists.txt
    ├── main.c
    ├── board_config.h
    ├── led_rgb.c
    ├── led_rgb.h
    ├── analog.c
    ├── analog.h
    ├── button.c
    └── button.h
```

## Descripción técnica breve

El programa se divide en módulos:

- `board_config.h`: define pines, constantes del NTC, umbrales de temperatura y tiempos.
- `analog.c/h`: configura el ADC y lee el potenciómetro y el NTC.
- `led_rgb.c/h`: controla los LED RGB usando PWM con LEDC.
- `button.c/h`: configura el botón con interrupción y cola de FreeRTOS.
- `main.c`: crea las tareas, maneja la máquina de estados y coordina todo el sistema.

El proyecto usa dos tareas principales de FreeRTOS:

- `temperature_task`: lee el NTC y actualiza el LED RGB de temperatura.
- `color_config_task`: lee el potenciómetro, procesa el botón y actualiza el LED RGB configurable.
