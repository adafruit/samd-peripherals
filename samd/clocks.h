/*
 * This file is part of the MicroPython project, http://micropython.org/
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

#ifndef MICROPY_INCLUDED_ATMEL_SAMD_CLOCKS_H
#define MICROPY_INCLUDED_ATMEL_SAMD_CLOCKS_H

#include <stdbool.h>
#include <stdint.h>

#include "include/sam.h"

#ifdef SAM_D5X_E5X
#define CLOCK_48MHZ GCLK_GENCTRL_SRC_DFLL_Val
#endif
#ifdef SAMD21
#define CLOCK_48MHZ GCLK_GENCTRL_SRC_DFLL48M_Val
#endif

// Pass to clock_init() if fine calibration not known.
#define DEFAULT_DFLL48M_FINE_CALIBRATION 512

#define CORE_GCLK 0

uint8_t find_free_gclk(uint16_t divisor);

bool gclk_enabled(uint8_t gclk);
void disable_gclk(uint8_t gclk);
void reset_gclks(void);

void connect_gclk_to_peripheral(uint8_t gclk, uint8_t peripheral);
void disconnect_gclk_from_peripheral(uint8_t gclk, uint8_t peripheral);

void enable_clock_generator(uint8_t gclk, uint32_t source, uint16_t divisor);
void disable_clock_generator(uint8_t gclk);

/**
 * @brief Called during port_init to setup system clocks.
 *
 * @param has_rtc_crystal Indicates that the board has a crystal for the real-time
 * clock (RTC). If true, uses the microcontroller's XOSC32k as the RTC's clock source.
 * For an individual board, this value is configured from BOARD_HAS_CRYSTAL in
 * mpconfigboard.h.
 *
 * @param xosc_freq The frequency of a connected external oscillator, or 0 if no
 * external oscillator is connected. Non-zero values should be the frequency
 * in Hertz (Hz) of an external oscillator connected to an XIN pin on this
 * microcontroller. This is currently only implemented for SAMx5x chips.
 * For an individual board, this value is configured from BOARD_XOSC_FREQ_HZ
 * in mpconfigboard.h.
 *
 * @param xosc_is_crystal Set to true if the external oscillator (XOSC) described by
 * `xosc_freq` is a crystal oscillator, or false if it is not. If there is no XOSC,
 * then `xosc_freq` should be set to 0, in which case this parameter is ignored.
 * This is currently only implemented for SAMx5x chips.
 * For an individual board, this value is configured from BOARD_XOSC_IS_CRYSTAL
 * in mpconfigboard.h.
 *
 * @param dfll48m_fine_calibration The fine calibration value for the DFLL48M.
 * Currently only implemented for SAMD21 chips, and only used if `has_rtc_crystal`
 * is false.
 */
void clock_init(bool has_rtc_crystal, uint32_t xosc_freq, bool xosc_is_crystal, uint32_t dfll48m_fine_calibration);
void init_dynamic_clocks(void);

bool clock_get_enabled(uint8_t type, uint8_t index);
bool clock_get_parent(uint8_t type, uint8_t index, uint8_t *p_type, uint8_t *p_index);
uint32_t clock_get_frequency(uint8_t type, uint8_t index);
uint32_t clock_get_calibration(uint8_t type, uint8_t index);
int clock_set_calibration(uint8_t type, uint8_t index, uint32_t val);

#endif  // MICROPY_INCLUDED_ATMEL_SAMD_CLOCKS_H
