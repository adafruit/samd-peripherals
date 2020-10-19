/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 by Scott Shawcroft for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef MICROPY_INCLUDED_ATMEL_SAMD_PERIPHERALS_SAMD21_PINS_H
#define MICROPY_INCLUDED_ATMEL_SAMD_PERIPHERALS_SAMD21_PINS_H

#include "include/sam.h"

#include "samd_peripherals_config.h"

#define MUX_C 2
#define MUX_D 3
#define MUX_E 4
#define MUX_F 5
#define PINMUX(pin, mux) ((((uint32_t) pin) << 16) | (mux))

#define NO_PIN PORT_BITS

// Pins in datasheet order.
#if defined(PIN_PA00) && !defined(IGNORE_PIN_PA00)
extern const mcu_pin_obj_t pin_PA00;
#endif
#if defined(PIN_PA01) && !defined(IGNORE_PIN_PA01)
extern const mcu_pin_obj_t pin_PA01;
#endif
#if defined(PIN_PA02) && !defined(IGNORE_PIN_PA02)
extern const mcu_pin_obj_t pin_PA02;
#endif
#if defined(PIN_PA03) && !defined(IGNORE_PIN_PA03)
extern const mcu_pin_obj_t pin_PA03;
#endif
#if defined(PIN_PB04) && !defined(IGNORE_PIN_PB04)
extern const mcu_pin_obj_t pin_PB04;
#endif
#if defined(PIN_PB05) && !defined(IGNORE_PIN_PB05)
extern const mcu_pin_obj_t pin_PB05;
#endif
#if defined(PIN_PB06) && !defined(IGNORE_PIN_PB06)
extern const mcu_pin_obj_t pin_PB06;
#endif
#if defined(PIN_PB07) && !defined(IGNORE_PIN_PB07)
extern const mcu_pin_obj_t pin_PB07;
#endif
#if defined(PIN_PB08) && !defined(IGNORE_PIN_PB08)
extern const mcu_pin_obj_t pin_PB08;
#endif
#if defined(PIN_PB09) && !defined(IGNORE_PIN_PB09)
extern const mcu_pin_obj_t pin_PB09;
#endif
#if defined(PIN_PA04) && !defined(IGNORE_PIN_PA04)
extern const mcu_pin_obj_t pin_PA04;
#endif
#if defined(PIN_PA05) && !defined(IGNORE_PIN_PA05)
extern const mcu_pin_obj_t pin_PA05;
#endif
#if defined(PIN_PA06) && !defined(IGNORE_PIN_PA06)
extern const mcu_pin_obj_t pin_PA06;
#endif
#if defined(PIN_PA07) && !defined(IGNORE_PIN_PA07)
extern const mcu_pin_obj_t pin_PA07;
#endif
#if defined(PIN_PA08) && !defined(IGNORE_PIN_PA08)
extern const mcu_pin_obj_t pin_PA08;
#endif
#if defined(PIN_PA09) && !defined(IGNORE_PIN_PA09)
extern const mcu_pin_obj_t pin_PA09;
#endif
#if defined(PIN_PA10) && !defined(IGNORE_PIN_PA10)
extern const mcu_pin_obj_t pin_PA10;
#endif
#if defined(PIN_PA11) && !defined(IGNORE_PIN_PA11)
extern const mcu_pin_obj_t pin_PA11;
#endif
#if defined(PIN_PB10) && !defined(IGNORE_PIN_PB10)
extern const mcu_pin_obj_t pin_PB10;
#endif
#if defined(PIN_PB11) && !defined(IGNORE_PIN_PB11)
extern const mcu_pin_obj_t pin_PB11;
#endif
#if defined(PIN_PB12) && !defined(IGNORE_PIN_PB12)
extern const mcu_pin_obj_t pin_PB12;
#endif
#if defined(PIN_PB13) && !defined(IGNORE_PIN_PB13)
extern const mcu_pin_obj_t pin_PB13;
#endif
#if defined(PIN_PB14) && !defined(IGNORE_PIN_PB14)
extern const mcu_pin_obj_t pin_PB14;
#endif

// Second page.
#if defined(PIN_PB15) && !defined(IGNORE_PIN_PB15)
extern const mcu_pin_obj_t pin_PB15;
#endif
#if defined(PIN_PA12) && !defined(IGNORE_PIN_PA12)
extern const mcu_pin_obj_t pin_PA12;
#endif
#if defined(PIN_PA13) && !defined(IGNORE_PIN_PA13)
extern const mcu_pin_obj_t pin_PA13;
#endif
#if defined(PIN_PA14) && !defined(IGNORE_PIN_PA14)
extern const mcu_pin_obj_t pin_PA14;
#endif
#if defined(PIN_PA15) && !defined(IGNORE_PIN_PA15)
extern const mcu_pin_obj_t pin_PA15;
#endif
#if defined(PIN_PA16) && !defined(IGNORE_PIN_PA16)
extern const mcu_pin_obj_t pin_PA16;
#endif
#if defined(PIN_PA17) && !defined(IGNORE_PIN_PA17)
extern const mcu_pin_obj_t pin_PA17;
#endif
#if defined(PIN_PA18) && !defined(IGNORE_PIN_PA18)
extern const mcu_pin_obj_t pin_PA18;
#endif
#if defined(PIN_PA19) && !defined(IGNORE_PIN_PA19)
extern const mcu_pin_obj_t pin_PA19;
#endif
#if defined(PIN_PB16) && !defined(IGNORE_PIN_PB16)
extern const mcu_pin_obj_t pin_PB16;
#endif
#if defined(PIN_PB17) && !defined(IGNORE_PIN_PB17)
extern const mcu_pin_obj_t pin_PB17;
#endif
#if defined(PIN_PA20) && !defined(IGNORE_PIN_PA20)
extern const mcu_pin_obj_t pin_PA20;
#endif
#if defined(PIN_PA21) && !defined(IGNORE_PIN_PA21)
extern const mcu_pin_obj_t pin_PA21;
#endif
#if defined(PIN_PA22) && !defined(IGNORE_PIN_PA22)
extern const mcu_pin_obj_t pin_PA22;
#endif
#if defined(PIN_PA23) && !defined(IGNORE_PIN_PA23)
extern const mcu_pin_obj_t pin_PA23;
#endif
#if defined(PIN_PA24) && !defined(IGNORE_PIN_PA24)
extern const mcu_pin_obj_t pin_PA24;
#endif
#if defined(PIN_PA25) && !defined(IGNORE_PIN_PA25)
extern const mcu_pin_obj_t pin_PA25;
#endif
#if defined(PIN_PB22) && !defined(IGNORE_PIN_PB22)
extern const mcu_pin_obj_t pin_PB22;
#endif
#if defined(PIN_PB23) && !defined(IGNORE_PIN_PB23)
extern const mcu_pin_obj_t pin_PB23;
#endif
#if defined(PIN_PA27) && !defined(IGNORE_PIN_PA27)
extern const mcu_pin_obj_t pin_PA27;
#endif
#if defined(PIN_PA28) && !defined(IGNORE_PIN_PA28)
extern const mcu_pin_obj_t pin_PA28;
#endif
#if defined(PIN_PA30) && !defined(IGNORE_PIN_PA30)
extern const mcu_pin_obj_t pin_PA30;
#endif
#if defined(PIN_PA31) && !defined(IGNORE_PIN_PA31)
extern const mcu_pin_obj_t pin_PA31;
#endif
#if defined(PIN_PB30) && !defined(IGNORE_PIN_PB30)
extern const mcu_pin_obj_t pin_PB30;
#endif
#if defined(PIN_PB31) && !defined(IGNORE_PIN_PB31)
extern const mcu_pin_obj_t pin_PB31;
#endif
#if defined(PIN_PB00) && !defined(IGNORE_PIN_PB00)
extern const mcu_pin_obj_t pin_PB00;
#endif
#if defined(PIN_PB01) && !defined(IGNORE_PIN_PB01)
extern const mcu_pin_obj_t pin_PB01;
#endif
#if defined(PIN_PB02) && !defined(IGNORE_PIN_PB02)
extern const mcu_pin_obj_t pin_PB02;
#endif
#if defined(PIN_PB03) && !defined(IGNORE_PIN_PB03)
extern const mcu_pin_obj_t pin_PB03;
#endif

// For SAMR
#if defined(PIN_PC16) && !defined(IGNORE_PIN_PC16)
extern const mcu_pin_obj_t pin_PC16;
#endif
#if defined(PIN_PC18) && !defined(IGNORE_PIN_PC18)
extern const mcu_pin_obj_t pin_PC18;
#endif
#if defined(PIN_PC19) && !defined(IGNORE_PIN_PC19)
extern const mcu_pin_obj_t pin_PC19;
#endif

#endif  // MICROPY_INCLUDED_ATMEL_SAMD_PERIPHERALS_SAMD21_PINS_H
