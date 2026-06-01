#include "temp_control.h"
#include "board_config.h"

#include <ctype.h>

/*
 * ============================================================
 * CONTROL DE UNIDAD Y PERIODO DE IMPRESIÓN
 * ============================================================
 */

void temp_control_init(
    temp_control_t *control
)
{
    control->unit = TEMP_UNIT_C;
    control->print_period_s = TEMP_PRINT_PERIOD_DEFAULT_S;
}

void temp_control_set_period_s(
    temp_control_t *control,
    uint32_t period_s
)
{
    if (period_s < TEMP_PRINT_PERIOD_MIN_S) {
        period_s = TEMP_PRINT_PERIOD_MIN_S;
    }

    if (period_s > TEMP_PRINT_PERIOD_MAX_S) {
        period_s = TEMP_PRINT_PERIOD_MAX_S;
    }

    control->print_period_s = period_s;
}

bool temp_control_set_unit(
    temp_control_t *control,
    temp_unit_t unit
)
{
    if (unit > TEMP_UNIT_K) {
        return false;
    }

    control->unit = unit;
    return true;
}

void temp_control_next_unit(
    temp_control_t *control
)
{
    if (control->unit == TEMP_UNIT_K) {
        control->unit = TEMP_UNIT_C;
    } else {
        control->unit++;
    }
}

float temp_control_convert_from_c(
    temp_control_t *control,
    float temperature_c
)
{
    switch (control->unit) {
        case TEMP_UNIT_F:
            return (temperature_c * 9.0f / 5.0f) + 32.0f;

        case TEMP_UNIT_K:
            return temperature_c + 273.15f;

        case TEMP_UNIT_C:
        default:
            return temperature_c;
    }
}

const char *temp_control_unit_symbol(
    temp_control_t *control
)
{
    switch (control->unit) {
        case TEMP_UNIT_F:
            return "°F";

        case TEMP_UNIT_K:
            return "K";

        case TEMP_UNIT_C:
        default:
            return "°C";
    }
}

bool temp_control_parse_unit(
    const char *text,
    temp_unit_t *unit
)
{
    if (text == NULL || unit == NULL) {
        return false;
    }

    char value = (char)toupper((unsigned char)text[0]);

    if (value == 'C') {
        *unit = TEMP_UNIT_C;
        return true;
    }

    if (value == 'F') {
        *unit = TEMP_UNIT_F;
        return true;
    }

    if (value == 'K') {
        *unit = TEMP_UNIT_K;
        return true;
    }

    return false;
}