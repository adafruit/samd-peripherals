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

#include "samd/clocks.h"

#include "hpl_gclk_config.h"

bool gclk_enabled(uint8_t gclk) {
    return GCLK->GENCTRL[gclk].bit.GENEN;
}

void disable_gclk(uint8_t gclk) {
    while ((GCLK->SYNCBUSY.vec.GENCTRL & (1 << gclk)) != 0) {}
    GCLK->GENCTRL[gclk].bit.GENEN = false;
    while ((GCLK->SYNCBUSY.vec.GENCTRL & (1 << gclk)) != 0) {}
}

void connect_gclk_to_peripheral(uint8_t gclk, uint8_t peripheral) {
    GCLK->PCHCTRL[peripheral].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN(gclk);
    while(GCLK->SYNCBUSY.reg != 0) {}
}

void disconnect_gclk_from_peripheral(uint8_t gclk, uint8_t peripheral) {
    GCLK->PCHCTRL[peripheral].reg = 0;
}

static void enable_clock_generator_sync(uint8_t gclk, uint32_t source, uint16_t divisor, bool sync) {
    uint32_t divsel = 0;
    // The datasheet says 8 bits and max value of 512, how is that possible?
    if (divisor > 255) { // Generator 1 has 16 bits
        divsel = GCLK_GENCTRL_DIVSEL;
        for (int i = 15; i > 0; i--) {
            if (divisor & (1 << i)) {
                divisor = i - 1;
                break;
            }
        }
    }

    GCLK->GENCTRL[gclk].reg = GCLK_GENCTRL_SRC(source) | GCLK_GENCTRL_DIV(divisor) | divsel | GCLK_GENCTRL_OE | GCLK_GENCTRL_GENEN;
    if (sync)
        while ((GCLK->SYNCBUSY.vec.GENCTRL & (1 << gclk)) != 0) {}
}

void enable_clock_generator(uint8_t gclk, uint32_t source, uint16_t divisor) {
    enable_clock_generator_sync(gclk, source, divisor, true);
}

void disable_clock_generator(uint8_t gclk) {
    GCLK->GENCTRL[gclk].reg = 0;
    while ((GCLK->SYNCBUSY.vec.GENCTRL & (1 << gclk)) != 0) {}
}

static void init_clock_source_osculp32k(void) {
    // Calibration value is loaded at startup
    OSC32KCTRL->OSCULP32K.bit.EN1K = 0;
    OSC32KCTRL->OSCULP32K.bit.EN32K = 1;
}

static void init_clock_source_xosc32k(void) {
    OSC32KCTRL->XOSC32K.reg = OSC32KCTRL_XOSC32K_ONDEMAND |
                              OSC32KCTRL_XOSC32K_EN32K |
                              OSC32KCTRL_XOSC32K_XTALEN |
                              OSC32KCTRL_XOSC32K_ENABLE |
                              OSC32KCTRL_XOSC32K_CGM(1);
}

/**
 * @brief Initialize the DPLL clock source, which sources the main system clock.
 */
static void init_clock_source_dpll0(uint32_t xosc_freq, bool xosc_is_crystal)
{
    bool has_xosc = (xosc_freq != 0);

    uint8_t refclk_setting;

    if (has_xosc) {
        // If we have an external oscillator, use that as DPLL0's REFCLK.
        uint8_t xtalen = xosc_is_crystal ? OSCCTRL_XOSCCTRL_XTALEN : 0;
        refclk_setting = OSCCTRL_DPLLCTRLB_REFCLK_XOSC0_Val;

        // When we're using an external clock source, we need to configure DPLL0 based on
        // the frequency of that external clock source.
        //
        // According to the datasheet (28.6.5.1), the frequency of DPLL0 is dependent on its REFCLK
        // by this formula:
        // f_DPLL0 = f_REFCLK * (LDR + 1 + (LDRFRAC / 32)).
        // If we have an external clock source, then we'll be setting REFCLK to XOSC0.
        //
        // Our desired output frequency for DPLL0 (f_DPLL0) is 120 MHz.
        // We currently require xosc_freq to be an integer factor of 120 MHz, so we can ignore LDRFRAC.
        // With LDRFRAC at 0, we just need to calculate LDR.
        // Rearranging the variables, we get this:
        // LDR = (f_DPLL0 / f_XOSC0) - 1
        // LDR = (120 MHz / f_XOSC0) - 1
        uint32_t ldr = (120000000 / xosc_freq) - 1;

        OSCCTRL->XOSCCTRL[0].reg = OSCCTRL_XOSCCTRL_ENABLE | xtalen;
        OSCCTRL->Dpll[0].DPLLRATIO.reg = OSCCTRL_DPLLRATIO_LDRFRAC(0) | OSCCTRL_DPLLRATIO_LDR(ldr);

    } else {
        // If we don't have an external oscillator, use GCLK 5 as DPLL0's REFLCK.
        refclk_setting = OSCCTRL_DPLLCTRLB_REFCLK_GCLK_Val;
        GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL0].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN(5);

        // We also need to configure DPLL0 based on the the frequency of its REFCLK,
        // which here is GCLK 5.
        // GCLK 5 itself uses the DFLL48M as its source, but with a divisor of 24.
        // So the output frequency of GCLK 5 is 2 MHz:
        // f_DFLL = 48 MHz
        // f_GCLK5 = 48 MHz / 24 = 2 MHz
        //
        // According to the datasheet (28.6.5.1), the frequency of DPLL0 is dependent on its REFCLK
        // by this formula:
        // f_DPLL0 = f_REFCLK * (LDR + 1 + (LDRFRAC / 32)).
        //
        // We want f_DPLL0 to be 120 MHz, and f_REFCLK is known beforehand as shown above.
        // If we ignore LDRFRAC and leave it at 0, then we can calculate LDR with:
        // LDR = (f_DPLL0 / f_REFCLK) - 1.
        // LDR = (120 MHz / 2 MHz) - 1 = 59
        OSCCTRL->Dpll[0].DPLLRATIO.reg = OSCCTRL_DPLLRATIO_LDRFRAC(0) | OSCCTRL_DPLLRATIO_LDR(59);
    }

    // Apply the REFCLK that was determined above.
    OSCCTRL->Dpll[0].DPLLCTRLB.reg = OSCCTRL_DPLLCTRLB_REFCLK(refclk_setting);
    // Enable this clock.
    OSCCTRL->Dpll[0].DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_ENABLE;

    while (!(OSCCTRL->Dpll[0].DPLLSTATUS.bit.LOCK || OSCCTRL->Dpll[0].DPLLSTATUS.bit.CLKRDY)) {}
}

void clock_init(bool has_rtc_crystal, uint32_t xosc_freq, bool xosc_is_crystal, uint32_t dfll48m_fine_calibration) {
    // DFLL48M is enabled by default
    // TODO: handle fine calibration data.

    init_clock_source_osculp32k();

    if (has_rtc_crystal) {
        init_clock_source_xosc32k();
        OSC32KCTRL->RTCCTRL.bit.RTCSEL = OSC32KCTRL_RTCCTRL_RTCSEL_XOSC32K_Val;
    } else {
        OSC32KCTRL->RTCCTRL.bit.RTCSEL = OSC32KCTRL_RTCCTRL_RTCSEL_ULP32K_Val;
    }

    MCLK->CPUDIV.reg = MCLK_CPUDIV_DIV(1);

    // Set GCLK_GEN[0], which is used for GCLK_MAIN, to use DPLL0 as its source.
    // DPLL0's REFCLK is set in the init_clock_source_dpll0() call below.
    enable_clock_generator_sync(0, GCLK_GENCTRL_SRC_DPLL0_Val, 1, false);
    enable_clock_generator_sync(1, GCLK_GENCTRL_SRC_DFLL_Val, 1, false);
    enable_clock_generator_sync(4, GCLK_GENCTRL_SRC_DPLL0_Val, 1, false);
    // Note(Qyriad): if !has_xosc, GCLK 5 is set as the REFCLK source for DPLL0 in
    // init_clock_source_dpll0(), but I don't know if GCLK 5 is used elsewhere too,
    // so I haven't made enabling GCLK 5 conditional on has_xosc here.
    enable_clock_generator_sync(5, GCLK_GENCTRL_SRC_DFLL_Val, 24, false);
    enable_clock_generator_sync(6, GCLK_GENCTRL_SRC_DFLL_Val, 4, false);

    init_clock_source_dpll0(xosc_freq, xosc_is_crystal);

    // Do this after all static clock init so that they aren't used dynamically.
    init_dynamic_clocks();
}

static bool clk_enabled(uint8_t clk) {
    return GCLK->PCHCTRL[clk].bit.CHEN;
}

static uint8_t clk_get_generator(uint8_t clk) {
    return GCLK->PCHCTRL[clk].bit.GEN;
}

static uint8_t generator_get_source(uint8_t gen) {
    return GCLK->GENCTRL[gen].bit.SRC;
}

static bool osc_enabled(uint8_t index) {
    switch (index) {
        case GCLK_SOURCE_XOSC0:
            return OSCCTRL->XOSCCTRL[0].bit.ENABLE;
        case GCLK_SOURCE_XOSC1:
            return OSCCTRL->XOSCCTRL[1].bit.ENABLE;
        case GCLK_SOURCE_OSCULP32K:
            return true;
        case GCLK_SOURCE_XOSC32K:
            return OSC32KCTRL->XOSC32K.bit.ENABLE;
        case GCLK_SOURCE_DFLL:
            return OSCCTRL->DFLLCTRLA.bit.ENABLE;
        case GCLK_SOURCE_DPLL0:
            return OSCCTRL->Dpll[0].DPLLCTRLA.bit.ENABLE;
        case GCLK_SOURCE_DPLL1:
            return OSCCTRL->Dpll[1].DPLLCTRLA.bit.ENABLE;
    };
    return false;
}

static uint32_t osc_get_source(uint8_t index) {
    uint8_t dpll_index = index - GCLK_SOURCE_DPLL0;
    uint32_t refclk = OSCCTRL->Dpll[dpll_index].DPLLCTRLB.bit.REFCLK;
    switch (refclk) {
        case 0x0:
            return generator_get_source(GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL0 + dpll_index].bit.GEN);
        case 0x1:
            return GCLK_SOURCE_XOSC32K;
        case 0x2:
            return GCLK_SOURCE_XOSC0;
        case 0x3:
            return GCLK_SOURCE_XOSC1;
    }
    return 0;
}

static uint32_t osc_get_frequency(uint8_t index);

static uint32_t generator_get_frequency(uint8_t gen) {
        uint8_t src = GCLK->GENCTRL[gen].bit.SRC;
        uint32_t div;
        if (GCLK->GENCTRL[gen].bit.DIVSEL) {
            div = 1 << (GCLK->GENCTRL[gen].bit.DIV + 1);
        } else {
            div = GCLK->GENCTRL[gen].bit.DIV;
            if (!div)
                div = 1;
        }

        return osc_get_frequency(src) / div;
}

static uint32_t dpll_get_frequency(uint8_t index) {
    uint8_t dpll_index = index - GCLK_SOURCE_DPLL0;
    uint32_t refclk = OSCCTRL->Dpll[dpll_index].DPLLCTRLB.bit.REFCLK;
    uint32_t freq;

    switch (refclk) {
        case 0x0: // GCLK
            freq = generator_get_frequency(GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL0 + dpll_index].bit.GEN);
            break;
        case 0x1: // XOSC32
            freq = 32768;
            break;
        case 0x2: // XOSC0
            // If XOSC0 is being used as DPLL0's REFCLK, then we can figure out XOSC0's frequency.
            if (OSCCTRL->Dpll[0].DPLLCTRLB.bit.REFCLK == OSCCTRL_DPLLCTRLB_REFCLK_XOSC0_Val) {
                freq = osc_get_frequency(GCLK_SOURCE_XOSC0);
            } else {
                freq = 0;
            }
            break;
        case 0x3: // XOSC1
        default:
            return 0; // unknown
    }

    return (freq * (OSCCTRL->Dpll[dpll_index].DPLLRATIO.bit.LDR + 1)) +
           (freq * OSCCTRL->Dpll[dpll_index].DPLLRATIO.bit.LDRFRAC / 32);
}

static uint32_t osc_get_frequency(uint8_t index) {
    switch (index) {
        case GCLK_SOURCE_XOSC0:
            // If we're using XOSC0 as the REFCLK for DPLLL0, we can calculate XOSC0's frequency.
            if (OSCCTRL->Dpll[0].DPLLCTRLB.bit.REFCLK == OSCCTRL_DPLLCTRLB_REFCLK_XOSC0_Val) {
                // From the datasheet (28.6.5.1), the frequency of DPLL0 is dependent on
                // its REFCLK by this formula:
                // f_DPLL0 = f_REFCLK * (LDR + 1 + (LDRFRAC / 32)).
                // We know f_DPLL is 120 MHz (the system clock rate).
                // We currently always set LDRFRAC to 0, and we can retrieve LDR from the
                // register its set in. Rearranging the variables, we get:
                // f_XOSC0 = f_DPLL0 / (LDR + 1).
                uint32_t ldr = OSCCTRL->Dpll[0].DPLLRATIO.bit.LDR;
                uint32_t f_dpll0 = 120000000;
                uint32_t f_xosc0 = f_dpll0 / (ldr + 1);
                return f_xosc0;
            }
            // Otherwise, we don't know.
            return 0;
        case GCLK_SOURCE_XOSC1:
            return 0; // unknown
        case GCLK_SOURCE_OSCULP32K:
        case GCLK_SOURCE_XOSC32K:
            return 32768;
        case GCLK_SOURCE_DFLL:
            return 48000000;
        case GCLK_SOURCE_DPLL0:
        case GCLK_SOURCE_DPLL1:
            return dpll_get_frequency(index);
    }
    return 0;
}

bool clock_get_enabled(uint8_t type, uint8_t index) {
    if (type == 0)
        return osc_enabled(index);
    if (type == 1)
        return clk_enabled(index);
    if (type == 2)
        return SysTick->CTRL & SysTick_CTRL_ENABLE_Msk;
    return false;
}

bool clock_get_parent(uint8_t type, uint8_t index, uint8_t *p_type, uint8_t *p_index) {
    if (type == 0 && osc_enabled(index)) {
         if (index == GCLK_SOURCE_DPLL0 || index == GCLK_SOURCE_DPLL1) {
            *p_type = 0;
            *p_index = osc_get_source(index);
            return true;
         }
         return false;
    }
    if (type == 1 && index <= 47 && clk_enabled(index)) {
        *p_type = 0;
        *p_index = generator_get_source(clk_get_generator(index));
        return true;
    }
    if (type == 2) {
        switch (index) {
            case 0:
            case 1:
                *p_type = 0;
                *p_index = generator_get_source(0);
                return true;
            case 2:
                *p_type = 0;
                switch (OSC32KCTRL->RTCCTRL.bit.RTCSEL) {
                    case 0:
                    case 1:
                        *p_index = GCLK_SOURCE_OSCULP32K;
                        return true;
                    case 4:
                    case 5:
                        *p_index = GCLK_SOURCE_XOSC32K;
                        return true;
                }
                return false;
        }
    }
    return false;
}

uint32_t clock_get_frequency(uint8_t type, uint8_t index) {
    if (type == 0) {
        return osc_get_frequency(index);
    }
    if (type == 1 && index <= 47 && clk_enabled(index)) {
        return generator_get_frequency(clk_get_generator(index));
    }
    if (type == 2) {
        switch (index) {
            case 0:
                return clock_get_frequency(0, generator_get_source(0)) / SysTick->LOAD;
            case 1:
                return clock_get_frequency(0, generator_get_source(0)) / MCLK->CPUDIV.bit.DIV;
            case 2:
                switch (OSC32KCTRL->RTCCTRL.bit.RTCSEL) {
                    case 0:
                    case 4:
                        return 1024;
                    case 1:
                    case 5:
                        return 32768;
                }
        }
    }
    return 0;
}

uint32_t clock_get_calibration(uint8_t type, uint8_t index) {
    if (type == 0) {
        switch (index) {
            case GCLK_SOURCE_OSCULP32K:
                return OSC32KCTRL->OSCULP32K.bit.CALIB;
        };
    }
    if (type == 2 && index == 0) {
        return SysTick->LOAD + 1;
    }
    return 0;
}

int clock_set_calibration(uint8_t type, uint8_t index, uint32_t val) {
    if (type == 0) {
        switch (index) {
            case GCLK_SOURCE_OSCULP32K:
                if (val > 0x3f)
                    return -1;
                OSC32KCTRL->OSCULP32K.bit.CALIB = val;
                return 0;
        };
    }
    if (type == 2 && index == 0) {
        if (val < 0x1000 || val > 0x1000000)
            return -1;
        SysTick->LOAD = val - 1;
        return 0;
    }
    return -2;
}
