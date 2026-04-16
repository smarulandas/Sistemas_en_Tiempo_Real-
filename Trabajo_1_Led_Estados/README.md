# Trabajo 1 - LED de 5 estados con botón

Se implementó un programa en C para el ESP32-C6 usando el LED integrado y el botón BOOT.

## Problema resuelto
Controlar el LED mediante 5 estados de funcionamiento y cambiar de estado cada vez que se presiona el botón.

## Estados
- Estado 1: encendido 2 s - apagado 2 s
- Estado 2: encendido 1 s - apagado 1 s
- Estado 3: encendido 0.5 s - apagado 0.5 s
- Estado 4: encendido 0.1 s - apagado 0.1 s
- Estado 5: apagado

## Implementación
La solución se realizó usando interrupciones y colas.
