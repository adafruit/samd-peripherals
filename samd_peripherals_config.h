/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
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

// Configuration options for the peripherals library. Copy into your project and edit
// it as needed. Ensure it's in an include path because the library does:
// #include "samd_peripherals_config.h"

#ifndef SAMD_PERIPHERALS_CONFIG_H
#define SAMD_PERIPHERALS_CONFIG_H

// Adds additional struct fields into mcu_pin_obj_t at the beginning. For example, CircuitPython
// uses this to add the Python type info into the struct.
#define PIN_PREFIX_FIELDS

// Values for the additional struct fields in mcu_pin_obj_t. They will be the same for all pins. For
// example, CircuitPython uses this to add the Python type info into the struct.
#define PIN_PREFIX_VALUES

#endif // SAMD_PERIPHERALS_CONFIG_H
