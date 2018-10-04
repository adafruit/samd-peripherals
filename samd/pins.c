#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "pins.h"

extern const mcu_pin_obj_t samd_pins[];
extern const int samd_pins_len;

const mcu_pin_obj_t* samd_peripherals_get_pin(uint8_t number)
{
    for (int i = 0; i < samd_pins_len; i++)
        if (samd_pins[i].number == number)
            return &samd_pins[i];

    return NULL;
}