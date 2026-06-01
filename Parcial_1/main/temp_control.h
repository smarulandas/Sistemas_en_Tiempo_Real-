#ifndef TEMP_CONTROL_H
#define TEMP_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

/*
 * ============================================================
 * UNIDADES DE TEMPERATURA
 * ============================================================
 *
 * Internamente se trabaja en °C.
 * La conversión solo se usa al imprimir.
 */
typedef enum {
    TEMP_UNIT_C = 0,
    TEMP_UNIT_F,
    TEMP_UNIT_K
} temp_unit_t;

/*
 * Guarda la unidad actual y el periodo de impresión.
 */
typedef struct {
    temp_unit_t unit;
    uint32_t print_period_s;
} temp_control_t;

/* Inicializa unidad en °C y periodo por defecto. */
void temp_control_init(
    temp_control_t *control
);

/* Cambia el periodo de impresión en segundos. */
void temp_control_set_period_s(
    temp_control_t *control,
    uint32_t period_s
);

/* Cambia la unidad de impresión. */
bool temp_control_set_unit(
    temp_control_t *control,
    temp_unit_t unit
);

/* Cambia la unidad en orden: °C -> °F -> K -> °C. */
void temp_control_next_unit(
    temp_control_t *control
);

/* Convierte una temperatura en °C a la unidad seleccionada. */
float temp_control_convert_from_c(
    temp_control_t *control,
    float temperature_c
);

/* Devuelve el símbolo de la unidad actual. */
const char *temp_control_unit_symbol(
    temp_control_t *control
);

/* Convierte texto C, F o K a una unidad válida. */
bool temp_control_parse_unit(
    const char *text,
    temp_unit_t *unit
);

#endif