/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2026 by Dan Halbert for Adafruit Industries
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

#include "adc.h"

// The ASF4 adc_sync routines take a channel argument, but the SAMD ADC has a single
// conversion path, so they ignore it.
#define IGNORED_CHANNEL 0

void samd_peripherals_adc_start(struct adc_sync_descriptor *adc, Adc *instance,
    uint8_t reference, uint8_t gain, uint8_t pos_input) {
    samd_peripherals_adc_setup(adc, instance);
    adc_sync_set_reference(adc, reference);
    #ifdef SAMD21
    adc_sync_set_channel_gain(adc, IGNORED_CHANNEL, gain);
    #else
    (void)gain;
    #endif
    adc_sync_set_resolution(adc, ADC_CTRLB_RESSEL_12BIT_Val);
    adc_sync_enable_channel(adc, IGNORED_CHANNEL);
    adc_sync_set_inputs(adc, pos_input, ADC_INPUTCTRL_MUXNEG_GND_Val, IGNORED_CHANNEL);
}

uint16_t samd_peripherals_adc_read(struct adc_sync_descriptor *adc) {
    // Read twice and discard first result, as recommended in section 14 of
    // http://www.atmel.com/images/Atmel-42645-ADC-Configurations-with-Examples_ApplicationNote_AT11481.pdf
    // "Discard the first conversion result whenever there is a change in ADC configuration
    // like voltage reference / ADC channel change"
    // Empirical observation shows the first reading is quite different than subsequent ones.
    uint16_t value;
    adc_sync_read_channel(adc, IGNORED_CHANNEL, ((uint8_t *)&value), 2);
    adc_sync_read_channel(adc, IGNORED_CHANNEL, ((uint8_t *)&value), 2);
    return value;
}
