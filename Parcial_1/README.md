# Parcial 1 - Sistemas en Tiempo Real

## Descripción general

Este proyecto corresponde al parcial de la asignatura **Sistemas en Tiempo Real**.
Se desarrolló sobre una tarjeta **ESP32-C6** usando **ESP-IDF**.

El sistema permite leer temperatura mediante un sensor NTC, controlar LEDs RGB mediante PWM, recibir comandos por UART, cambiar unidades de temperatura con un botón físico y usar un potenciómetro para definir un umbral de activación de un LED rojo.

El proyecto fue organizado por módulos para mantener el código más claro, reutilizable y fácil de explicar.

---

## Objetivos del proyecto

El trabajo debía cumplir los siguientes puntos:

1. Imprimir la temperatura cada X segundos, donde X se puede cambiar mediante un comando UART.
2. Cambiar la unidad de impresión de temperatura entre °C, °F y Kelvin mediante comando UART.
3. Encender un LED RGB1 según rangos de temperatura:

   * Rojo entre un valor mínimo y máximo.
   * Verde entre un valor mínimo y máximo.
   * Azul entre un valor mínimo y máximo.
4. Permitir configurar la intensidad de los colores rojo, verde y azul.
5. Cambiar la unidad de impresión de temperatura usando un botón físico.
6. Usar un potenciómetro para seleccionar un umbral entre 0 y 100.
7. Leer el umbral del potenciómetro mediante un comando UART.
8. Encender un LED rojo según el umbral seleccionado.
9. Mantener el código organizado, comentado y sin variables globales de estado.

---

## Funcionamiento general

El programa trabaja con dos tareas principales:

### 1. Tarea de comandos UART

Esta tarea lee los comandos escritos desde el monitor serial.
El usuario escribe un comando, presiona ENTER y el programa interpreta la instrucción.

Los comandos permiten cambiar el periodo de impresión, la unidad de temperatura, los rangos del LED RGB1, las intensidades de color y consultar el umbral del potenciómetro.

### 2. Tarea de control

Esta tarea realiza el funcionamiento principal del sistema:

* Lee la temperatura del sensor NTC.
* Lee el valor del potenciómetro.
* Actualiza el LED RGB1 según los rangos de temperatura.
* Controla el LED rojo asociado al umbral.
* Revisa si el botón fue presionado.
* Cambia la unidad de temperatura cuando se presiona el botón.
* Imprime la temperatura cada cierto tiempo configurable.

---

## Comandos UART disponibles

Los comandos se escriben desde el monitor serial.

### Mostrar ayuda

```text
help
```

Muestra la lista de comandos disponibles.

### Cambiar periodo de impresión

```text
periodo 5
```

Configura la impresión de temperatura cada 5 segundos.

### Cambiar unidad de temperatura

```text
unidad C
unidad F
unidad K
```

Permite imprimir la temperatura en Celsius, Fahrenheit o Kelvin.

### Configurar rangos del RGB1

```text
rgb1_rango red 5 20
rgb1_rango blue 10 30
rgb1_rango green 15 40
```

Con estos comandos se definen los rangos de temperatura para cada color.

Ejemplo:

* Rojo entre 5 °C y 20 °C.
* Azul entre 10 °C y 30 °C.
* Verde entre 15 °C y 40 °C.

Si la temperatura está dentro de varios rangos al mismo tiempo, pueden encenderse varios colores simultáneamente.

### Configurar intensidad del RGB1

```text
rgb1_int red 80
rgb1_int blue 60
rgb1_int green 100
```

Configura la intensidad de cada color del RGB1.

### Configurar intensidad roja del RGB2

```text
rgb2_int red 70
```

Configura la intensidad del canal rojo usado para el indicador de umbral.

### Leer umbral del potenciómetro

```text
umbral
```

Muestra el valor actual del umbral seleccionado con el potenciómetro.

---

## Lógica del potenciómetro

El potenciómetro se lee mediante el ADC del ESP32-C6.
El valor leído se convierte a un rango entre 0 y 100.

Ese valor se interpreta como un umbral.
Luego el programa compara la temperatura actual con ese umbral.

La lógica usada es:

```text
Si temperatura_actual >= umbral
    Se enciende el LED rojo
Si temperatura_actual < umbral
    Se apaga el LED rojo
```

De esta forma, el potenciómetro permite modificar físicamente el punto de activación del LED rojo.

---

## Botón físico

El botón permite cambiar la unidad de impresión de temperatura sin usar comandos UART.

Cada vez que se presiona el botón, la unidad cambia en este orden:

```text
°C -> °F -> K -> °C
```

---

## LED RGB de ánodo común

Los LEDs RGB usados son de **ánodo común**.

Por esta razón, en la configuración del proyecto se usa:

```c
#define RGB_ACTIVE_HIGH 0
```

Esto significa que el LED se enciende con nivel bajo en el GPIO, ya que la pata común del RGB va conectada a 3.3 V.

Conexión general para un RGB de ánodo común:

```text
Pata común del RGB -> 3.3 V
Rojo  -> resistencia -> GPIO correspondiente
Verde -> resistencia -> GPIO correspondiente
Azul  -> resistencia -> GPIO correspondiente
```

---

## Organización del código

El proyecto está dividido en varios archivos para separar responsabilidades:

```text
main.c
```

Archivo principal del programa.
Crea las tareas, inicializa los módulos y conecta toda la lógica del sistema.

```text
app_commands.c / app_commands.h
```

Contiene la interpretación de comandos UART.

```text
analog.c / analog.h
```

Contiene la lectura del sensor NTC y del potenciómetro mediante ADC.

```text
rgb_led.c / rgb_led.h
```

Contiene el control PWM de los LEDs RGB.

```text
temp_control.c / temp_control.h
```

Contiene la lógica relacionada con temperatura, unidades y periodo de impresión.

```text
button_unit.c / button_unit.h
```

Contiene la lectura del botón físico.

```text
board_config.h
```

Contiene la configuración general del hardware, pines, constantes y parámetros iniciales.

---

## Requisitos importantes cumplidos

* Lectura de temperatura con NTC.
* Impresión de temperatura cada X segundos.
* Cambio de unidad por UART.
* Cambio de unidad por botón físico.
* Control de LED RGB1 por rangos de temperatura.
* Intensidad configurable para los colores del RGB.
* Potenciómetro usado como umbral entre 0 y 100.
* Comando UART para consultar el umbral.
* Configuración para LEDs RGB de ánodo común.
* Código separado por módulos.
* Variables con nombres claros.
* Sin variables globales de estado para la lógica principal del sistema.

---

## Compilación y ejecución

Para compilar el proyecto:

```powershell
idf.py build
```

Para cargarlo en la ESP32-C6:

```powershell
idf.py flash
```

Para abrir el monitor serial:

```powershell
idf.py monitor
```

También se puede usar:

```powershell
idf.py flash monitor
```

---

## Autor

Proyecto realizado para la asignatura **Sistemas en Tiempo Real**.
