/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Dan Halbert for Adafruit Industries
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

#include "sam.h"

// Turn off cache and invalidate all data in it.
void samd_peripherals_disable_and_clear_cache(void) {
    // Memory fence for hardware and compiler reasons. If this routine is inlined, the compiler
    // needs to know that everything written out be stored before this is called.
    // -O2 optimization showed this was necessary.
    __sync_synchronize();

    CMCC->CTRL.bit.CEN = 0;
    while (CMCC->SR.bit.CSTS) {}
    CMCC->MAINT0.bit.INVALL = 1;

    // Memory fence for hardware and compiler reasons. Testing showed this second one is also
    // necessary when compiled with -O2.
    __sync_synchronize();
}

// Enable cache
void samd_peripherals_enable_cache(void) {
    CMCC->CTRL.bit.CEN = 1;
}
