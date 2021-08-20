/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
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
#include "samd/dma.h"

#include <string.h>

#include "py/gc.h"
#include "py/mpstate.h"

#include "hal/utils/include/utils.h"

#include "shared-bindings/microcontroller/__init__.h"

COMPILER_ALIGNED(16) static DmacDescriptor dma_descriptors[DMA_CHANNEL_COUNT];

// Don't use these directly. They are used by the DMA engine itself.
COMPILER_ALIGNED(16) static DmacDescriptor write_back_descriptors[DMA_CHANNEL_COUNT];

#ifdef SAMD21
#define FIRST_SERCOM_RX_TRIGSRC 0x01
#define FIRST_SERCOM_TX_TRIGSRC 0x02
#endif
#ifdef SAM_D5X_E5X
#define FIRST_SERCOM_RX_TRIGSRC 0x04
#define FIRST_SERCOM_TX_TRIGSRC 0x05
#endif

void init_shared_dma(void) {
    // Turn on the clocks
    #ifdef SAM_D5X_E5X
    MCLK->AHBMASK.reg |= MCLK_AHBMASK_DMAC;
    #endif

    #ifdef SAMD21
    PM->AHBMASK.reg |= PM_AHBMASK_DMAC;
    PM->APBBMASK.reg |= PM_APBBMASK_DMAC;
    #endif

    DMAC->CTRL.reg = DMAC_CTRL_SWRST;

    DMAC->BASEADDR.reg = (uint32_t) dma_descriptors;
    DMAC->WRBADDR.reg = (uint32_t) write_back_descriptors;

    DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN0;

    for (uint8_t i = 0; i < AUDIO_DMA_CHANNEL_COUNT; i++) {
        dma_configure(i, 0, true);
    }
}

// Do write and read simultaneously. If buffer_out is NULL, write the tx byte over and over.
// If buffer_out is a real buffer, ignore tx.
// DMAs buffer_out -> dest
// DMAs src -> buffer_in
static int32_t shared_dma_transfer(void* peripheral,
                                   const uint8_t* buffer_out, volatile uint32_t* dest,
                                   volatile uint32_t* src, uint8_t* buffer_in,
                                   uint32_t length, uint8_t tx) {
    if (!dma_channel_free(SHARED_TX_CHANNEL) ||
        (buffer_in != NULL && !dma_channel_free(SHARED_RX_CHANNEL))) {
        return -1;
    }

    uint32_t beat_size = DMAC_BTCTRL_BEATSIZE_BYTE;
    bool sercom = true;
    bool tx_active = false;
    bool rx_active = false;
    uint16_t beat_length = length;
    #ifdef SAM_D5X_E5X
    if (peripheral == QSPI) {
        // Check input alignment on word boundaries.
        if ((((uint32_t) buffer_in) & 0x3) != 0 ||
            (((uint32_t) buffer_out) & 0x3) != 0) {
            return -3;
        }
        beat_size = DMAC_BTCTRL_BEATSIZE_WORD | DMAC_BTCTRL_SRCINC | DMAC_BTCTRL_DSTINC;
        beat_length /= 4;
        sercom = false;
        if (buffer_out != NULL) {
            dma_configure(SHARED_TX_CHANNEL, QSPI_DMAC_ID_TX, false);
            tx_active = true;
        } else {
            dma_configure(SHARED_RX_CHANNEL, QSPI_DMAC_ID_RX, false);
            rx_active = true;
        }

    } else {
    #endif

        dma_configure(SHARED_TX_CHANNEL, sercom_index(peripheral) * 2 + FIRST_SERCOM_TX_TRIGSRC, false);
        tx_active = true;
        if (buffer_in != NULL) {
            dma_configure(SHARED_RX_CHANNEL, sercom_index(peripheral) * 2 + FIRST_SERCOM_RX_TRIGSRC, false);
            rx_active = true;
        }

    #ifdef SAM_D5X_E5X
    }
    #endif

    // Set up RX first.
    if (rx_active) {
        DmacDescriptor* rx_descriptor = &dma_descriptors[SHARED_RX_CHANNEL];
        rx_descriptor->BTCTRL.reg = beat_size | DMAC_BTCTRL_DSTINC;
        rx_descriptor->BTCNT.reg = beat_length;
        rx_descriptor->SRCADDR.reg = ((uint32_t) src);
        #ifdef SAM_D5X_E5X
        if (peripheral == QSPI) {
            rx_descriptor->SRCADDR.reg = ((uint32_t) src + length);
        }
        #endif
        rx_descriptor->DSTADDR.reg = ((uint32_t)buffer_in + length);
        rx_descriptor->BTCTRL.bit.VALID = true;
    }

    // Set up TX second.
    if (tx_active) {
        DmacDescriptor* tx_descriptor = &dma_descriptors[SHARED_TX_CHANNEL];
        tx_descriptor->BTCTRL.reg = beat_size;
        tx_descriptor->BTCNT.reg = beat_length;

        if (buffer_out != NULL) {
            tx_descriptor->SRCADDR.reg = ((uint32_t)buffer_out + length);
            tx_descriptor->BTCTRL.reg |= DMAC_BTCTRL_SRCINC;
        } else {
            tx_descriptor->SRCADDR.reg = ((uint32_t) &tx);
        }
        tx_descriptor->DSTADDR.reg = ((uint32_t) dest);
        tx_descriptor->BTCTRL.bit.VALID = true;
    }
    if (sercom) {
        SercomSpi *s = &((Sercom*) peripheral)->SPI;
        s->INTFLAG.reg = SERCOM_SPI_INTFLAG_RXC | SERCOM_SPI_INTFLAG_DRE;
    }
    // Start the RX job first so we don't miss the first byte. The TX job clocks
    // the output.
    if (rx_active) {
        dma_enable_channel(SHARED_RX_CHANNEL);
    }
    if (tx_active) {
        dma_enable_channel(SHARED_TX_CHANNEL);
    }


    if (!sercom) {
        if (rx_active) {
            DMAC->SWTRIGCTRL.reg |= (1 << SHARED_RX_CHANNEL);
        }
    }

    #ifdef SAM_D5X_E5X
    // Sometimes (silicon bug?) this DMA transfer never starts, and another channel sits with
    // CHSTATUS.reg = 0x3 (BUSY | PENDING).  On the other hand, this is a
    // legitimate state for a DMA channel to be in (apparently), so we can't use that alone as a check.
    // Instead, let's look at the ACTIVE flag.  When DMA is hung, everything in ACTIVE is zeros.
    bool is_okay = false;
    for (int i=0; i<10 && !is_okay; i++) {
        bool complete = true;
        if (rx_active) {
            if (DMAC->Channel[SHARED_RX_CHANNEL].CHSTATUS.reg & 0x3)
                complete = false;
        }
        if (tx_active) {
            if (DMAC->Channel[SHARED_TX_CHANNEL].CHSTATUS.reg & 0x3)
                complete = false;
        }
        is_okay = is_okay || (DMAC->ACTIVE.bit.ABUSY || complete);
    }
    if (!is_okay) {
        for (int i=0; i<AUDIO_DMA_CHANNEL_COUNT; i++) {
            if(DMAC->Channel[i].CHCTRLA.bit.ENABLE) {
                DMAC->Channel[i].CHCTRLA.bit.ENABLE = 0;
                DMAC->Channel[i].CHCTRLA.bit.ENABLE = 1;
            }
        }
    }
    #endif

    // busy-wait for the RX and TX DMAs to either complete or encounter an error
    if (rx_active) {
        while ((dma_transfer_status(SHARED_RX_CHANNEL) & 0x3) == 0) {}
    }
    if (tx_active) {
        while ((dma_transfer_status(SHARED_TX_CHANNEL) & 0x3) == 0) {}
    }

    if (sercom) {
        Sercom* s = (Sercom*) peripheral;
        // Wait for the SPI transfer to complete.
        while (s->SPI.INTFLAG.bit.TXC == 0) {}

        // This transmit will cause the RX buffer overflow but we're OK with that.
        // So, read the garbage and clear the overflow flag.
        if (!rx_active) {
            while (s->SPI.INTFLAG.bit.RXC == 1) {
                s->SPI.DATA.reg;
            }
            s->SPI.STATUS.bit.BUFOVF = 1;
            s->SPI.INTFLAG.reg = SERCOM_SPI_INTFLAG_ERROR;
        }
    }

    if ((!rx_active || dma_transfer_status(SHARED_RX_CHANNEL) == DMAC_CHINTFLAG_TCMPL) &&
        (!tx_active || dma_transfer_status(SHARED_TX_CHANNEL) == DMAC_CHINTFLAG_TCMPL)) {
        return length;
    }
    return -2;
}


int32_t sercom_dma_transfer(Sercom* sercom, const uint8_t* buffer_out, uint8_t* buffer_in,
                            uint32_t length) {
    return shared_dma_transfer(sercom, buffer_out, &sercom->SPI.DATA.reg, &sercom->SPI.DATA.reg, buffer_in, length, 0);
}

int32_t sercom_dma_write(Sercom* sercom, const uint8_t* buffer, uint32_t length) {
    return shared_dma_transfer(sercom, buffer, &sercom->SPI.DATA.reg, NULL, NULL, length, 0);
}

int32_t sercom_dma_read(Sercom* sercom, uint8_t* buffer, uint32_t length, uint8_t tx) {
    return shared_dma_transfer(sercom, NULL, &sercom->SPI.DATA.reg, &sercom->SPI.DATA.reg, buffer, length, tx);
}

#ifdef SAM_D5X_E5X
int32_t qspi_dma_write(uint32_t address, const uint8_t* buffer, uint32_t length) {
    return shared_dma_transfer(QSPI, buffer, (uint32_t*) (QSPI_AHB + address), NULL, NULL, length, 0);
}

int32_t qspi_dma_read(uint32_t address, uint8_t* buffer, uint32_t length) {
    return shared_dma_transfer(QSPI, NULL, NULL, (uint32_t*) (QSPI_AHB + address), buffer, length, 0);
}
#endif

DmacDescriptor* dma_descriptor(uint8_t channel_number) {
    return &dma_descriptors[channel_number];
}

DmacDescriptor* dma_write_back_descriptor(uint8_t channel_number) {
    return &write_back_descriptors[channel_number];
}
