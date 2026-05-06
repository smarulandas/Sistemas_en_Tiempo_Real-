# CMakeLists.txt del componente main.
# Este archivo indica qué archivos .c hacen parte del programa principal.
idf_component_register(
    SRCS
        "main.c"        # Archivo principal: crea tareas y coordina el proyecto.
        "led_rgb.c"     # Módulo para controlar LED RGB por PWM.
        "analog.c"      # Módulo para leer potenciómetro y NTC con ADC.
        "button.c"      # Módulo para manejar botón con interrupción.
    INCLUDE_DIRS
        "."             # Permite incluir headers de esta misma carpeta.
    REQUIRES
        esp_driver_gpio # Necesario para GPIO y botón.
        esp_driver_ledc # Necesario para PWM LEDC.
        esp_adc         # Necesario para ADC oneshot.
)
