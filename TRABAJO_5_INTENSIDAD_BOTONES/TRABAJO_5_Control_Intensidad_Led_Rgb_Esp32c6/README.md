# Trabajo 5 - Control de intensidad de LED RGB con 3 botones usando ESP32-C6

## Resumen de lo que se hizo en el código

Se desarrolló un programa en C para ESP32-C6 usando ESP-IDF, donde se controla un LED RGB mediante 3 botones.

La lógica se organizó en forma de librería propia, separando el código en:

- `rgb_led.h`: declaraciones, estructura y prototipos
- `rgb_led.c`: implementación de funciones del LED RGB
- `main.c`: programa principal, donde se leen los botones y se llama la librería

### Funcionamiento general

- Hay 3 botones
- Cada botón controla un color del LED RGB:
  - botón 1: rojo
  - botón 2: verde
  - botón 3: azul
- Cada vez que se presiona un botón, el color correspondiente aumenta 10%
- Cuando llega a 100%, en la siguiente pulsación vuelve a 0%
- El estado del LED se muestra por monitor serial, por ejemplo:
  - `R=0% G=0% B=10%`

---

## Aspectos clave del desarrollo

### 1. Separación del código en `.h`, `.c` y `main.c`

Se separó el proyecto en una librería propia para manejar el LED RGB y un archivo principal para la lógica de la aplicación. Esto permite que el código quede más organizado, modular y reutilizable.

### 2. Uso de una estructura `rgb_led_t`

Se creó una estructura para guardar toda la configuración del LED RGB en una sola variable: pines, canales PWM, frecuencia, resolución y porcentajes de color. Esto evita usar muchas variables sueltas y mejora la claridad del código.

### 3. Uso de PWM para controlar la intensidad

Para cambiar la intensidad del LED no basta con encender y apagar un pin. Se usó PWM para variar el brillo de cada canal del LED RGB en diferentes porcentajes.

### 4. Uso del periférico LEDC

Se utilizó el periférico LEDC del ESP32-C6 para generar las señales PWM de los canales rojo, verde y azul. Este periférico es adecuado para controlar brillo en LEDs.

### 5. Manejo de intensidades en porcentaje

La lógica del programa trabaja con porcentajes de brillo: 0%, 10%, 20%, etc. Internamente esos porcentajes se convierten a duty cycle PWM. Esto hace el código más entendible y más fácil de usar.

### 6. Manejo de LED RGB ánodo común

El LED utilizado es de ánodo común, ya que la pata más larga corresponde al positivo común. Por eso se trabajó con lógica invertida usando `active_low = true`, lo que permite que el control de brillo funcione correctamente.

### 7. Botones conectados con pull-up interno

Los botones se configuraron como entradas con pull-up interno. Por eso cada botón se conecta entre el GPIO y GND, sin necesidad de usar resistencias externas para los botones.

### 8. Implementación de debounce por software

Se implementó debounce por software para evitar que una sola pulsación del botón sea detectada varias veces por los rebotes mecánicos del pulsador. Para esto se usó control de tiempo con `esp_timer_get_time()`.

### 9. Uso de sondeo en lugar de interrupciones

Para este ejercicio se utilizó lectura periódica de botones en lugar de interrupciones. Esta solución es suficiente para una aplicación sencilla y además es más fácil de explicar, probar y depurar.

### 10. Función `rgb_led_step_up()`

Se creó una función para aumentar el porcentaje de intensidad de un color en pasos de 10%. En la versión final, cuando el porcentaje llega a 100%, la siguiente pulsación lo regresa a 0%.

### 11. Función para mostrar el estado RGB

Se centralizó la impresión del estado del LED RGB en una función dedicada. Así, cada vez que se presiona un botón, el sistema muestra claramente el valor actual de rojo, verde y azul por monitor serial.

### 12. Adaptabilidad a otros tipos de LED RGB

El código fue pensado para poder adaptarse tanto a ánodo común como a cátodo común. En este proyecto se usó ánodo común, pero con un pequeño cambio en la lógica (`active_low`) también puede funcionar con cátodo común.

### 13. Modularidad del proyecto

No se dejó toda la lógica dentro de `main.c`. El manejo del LED RGB se separó en una librería específica, mientras que `main.c` solo coordina la aplicación, los botones y el flujo principal del programa. Esto mejora la organización y facilita el mantenimiento.

---

## Estructura del proyecto

```text
trabajo_5_control_intensidad_led_rgb_esp32c6
├── main
│   ├── main.c
│   ├── rgb_led.c
│   ├── rgb_led.h
│   └── CMakeLists.txt
├── CMakeLists.txt
├── sdkconfig
├── README.md
└── .gitignore