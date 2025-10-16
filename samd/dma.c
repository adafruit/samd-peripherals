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
#include "py/mphal.h"
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

static bool dma_allocated[DMA_CHANNEL_COUNT];

uint8_t dma_allocate_audio_channel(void) {
    for (uint8_t channel = 0; channel < AUDIO_DMA_CHANNEL_COUNT; channel++) {
        if (!dma_allocated[channel]) {
            dma_allocated[channel] = true;
            return channel;
        }
    }
    return NO_DMA_CHANNEL;
}

uint8_t dma_allocate_non_audio_channel(void) {
    // Start allocating above audio channels.
    for (uint8_t channel = AUDIO_DMA_CHANNEL_COUNT; channel < DMA_CHANNEL_COUNT; channel++) {
        if (!dma_allocated[channel]) {
            dma_allocated[channel] = true;
            return channel;
        }
    }
    return NO_DMA_CHANNEL;
}

void dma_free_channel(uint8_t channel) {
    if (channel == NO_DMA_CHANNEL) {
        return;
    }
    assert(dma_allocated[channel]);
    dma_disable_channel(channel);
    dma_allocated[channel] = false;
}

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

    // Configure audio channels in advance.
    // Non-audio channels will be configured on demand.
    for (uint8_t i = 0; i < AUDIO_DMA_CHANNEL_COUNT; i++) {
        dma_configure(i, 0, true);
    }
}

// Do write and read simultaneously. If buffer_out is NULL, write the tx byte over and over.
// If buffer_out is a real buffer, ignore tx.
// DMAs buffer_out -> dest
// DMAs src -> buffer_in
void shared_dma_transfer_start(dma_transfer_t *transfer, void* peripheral, const uint8_t* buffer_out, volatile uint32_t* dest, volatile uint32_t* src, uint8_t* buffer_in, uint32_t length, uint8_t tx) {
    transfer->progress = 0;

    // There's always a tx DMA channel, though it may be sending only the tx byte if buffer_out is NULL.
    uint8_t tx_channel = dma_allocate_non_audio_channel();
    if (tx_channel == NO_DMA_CHANNEL) {
        transfer->failure = DMA_FAILURE_NO_CHANNEL_AVAILABLE;
        return;
    }

    // Allocate an rx dma channel only if we're going to read.
    uint8_t rx_channel = NO_DMA_CHANNEL;
    if (buffer_in != NULL) {
        rx_channel = dma_allocate_non_audio_channel();
        if (rx_channel == NO_DMA_CHANNEL) {
            transfer->failure = DMA_FAILURE_NO_CHANNEL_AVAILABLE;
            return;
        }
    }

    uint32_t beat_size = DMAC_BTCTRL_BEATSIZE_BYTE;
    bool sercom = true;
    bool tx_active = false;
    bool rx_active = false;
    uint16_t beat_length = length;
    #ifdef SAM_D5X_E5X
    if (peripheral == QSPI) {
        // Check input alignment on word boundaries.
        // NULLs will pass this test, so no need to check for them separately.
        if ((((uint32_t) buffer_in) & 0x3) != 0 ||
            (((uint32_t) buffer_out) & 0x3) != 0) {
            transfer->failure = DMA_FAILURE_ALIGNMENT;
            return;
        }
        beat_size = DMAC_BTCTRL_BEATSIZE_WORD | DMAC_BTCTRL_SRCINC | DMAC_BTCTRL_DSTINC;
        beat_length /= 4;
        sercom = false;
        if (buffer_out != NULL) {
            dma_configure(tx_channel, QSPI_DMAC_ID_TX, false);
            tx_active = true;
        } else {
            dma_configure(rx_channel, QSPI_DMAC_ID_RX, false);
            rx_active = true;
        }

    } else {
    #endif

        // There is always a TX channel.
        dma_configure(tx_channel, sercom_index(peripheral) * 2 + FIRST_SERCOM_TX_TRIGSRC, false);
        tx_active = true;
        if (rx_channel != NO_DMA_CHANNEL) {
            dma_configure(rx_channel, sercom_index(peripheral) * 2 + FIRST_SERCOM_RX_TRIGSRC, false);
            rx_active = true;
        }

    #ifdef SAM_D5X_E5X
    }
    #endif

    // Set up RX first.
    if (rx_active) {
        DmacDescriptor* rx_descriptor = &dma_descriptors[rx_channel];
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
        DmacDescriptor* tx_descriptor = &dma_descriptors[tx_channel];
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
        // TODO: test if this operation is necessary or if it's just a waste of time and space
        // Section 35.8.7 of the datasheet lists both of these bits as read-only, so this shouldn't do anything
        s->INTFLAG.reg = SERCOM_SPI_INTFLAG_RXC | SERCOM_SPI_INTFLAG_DRE;
    }

    // Start the RX job first so we don't miss the first byte. The TX job clocks the output.
    // Disable interrupts during startup to make sure both RX and TX start at just about the same time.
    mp_hal_disable_all_interrupts();
    if (rx_active) {
        dma_enable_channel(rx_channel);
    }
    if (tx_active) {
        dma_enable_channel(tx_channel);
    }
    mp_hal_enable_all_interrupts();

    if (!sercom) {
        if (rx_active) {
            DMAC->SWTRIGCTRL.reg |= (1 << rx_channel);
        }
    }

    #ifdef SAM_D5X_E5X
    // Sometimes (silicon bug?) this DMA transfer never starts, and another channel sits with
    // CHSTATUS.reg = 0x3 (BUSY | PENDING).  On the other hand, this is a
    // legitimate state for a DMA channel to be in (apparently), so we can't use that alone as a check.
    // Instead, let's look at the ACTIVE flag.  When DMA is hung, everything in ACTIVE is zeros.
    bool is_okay = false;
    for (int i = 0; i < 10 && !is_okay; i++) {
        bool complete = true;
        if (rx_active) {
            if (DMAC->Channel[rx_channel].CHSTATUS.reg & 0x3)
                complete = false;
        }
        if (tx_active) {
            if (DMAC->Channel[tx_channel].CHSTATUS.reg & 0x3)
                complete = false;
        }
        is_okay = is_okay || (DMAC->ACTIVE.bit.ABUSY || complete);
    }
    if (!is_okay) {
        for (int i = 0; i < DMA_CHANNEL_COUNT; i++) {
            if(DMAC->Channel[i].CHCTRLA.bit.ENABLE) {
                DMAC->Channel[i].CHCTRLA.bit.ENABLE = 0;
                DMAC->Channel[i].CHCTRLA.bit.ENABLE = 1;
            }
        }
    }
    #endif
    transfer->peripheral = peripheral;
    transfer->length = length;
    transfer->rx_channel = rx_channel;
    transfer->tx_channel = tx_channel;
    transfer->rx_active = rx_active;
    transfer->tx_active = tx_active;
    transfer->sercom = sercom;
    transfer->failure = 0;
    transfer->progress = 0;
}

bool shared_dma_transfer_finished(dma_transfer_t* transfer) {
    if (transfer->failure != 0) {
        return true;
    }

    if (transfer->progress < 1 && transfer->rx_active) {
        if ((dma_transfer_status(transfer->rx_channel) & 0x3) == 0) {
            // RX hasn't finished.
            return false;
        }
        // RX has finished
        transfer->progress = 1;
    }
    if (transfer->progress < 2 && transfer->tx_active) {
        if ((dma_transfer_status(transfer->tx_channel) & 0x3) == 0) {
            // TX hasn't finished.
            return false;
        }
        // TX has finished. RX has also finished, or there is no RX.
        transfer->progress = 2;
    }

    if (transfer->progress < 3 && transfer->sercom) {
        Sercom* s = (Sercom*) transfer->peripheral;
        // Wait for the SPI transfer to complete.
        if (s->SPI.INTFLAG.bit.TXC == 0) {
            return false;
        }
        transfer->progress = 3;

        // This transmit will cause the RX buffer overflow but we're OK with that.
        // So, read the garbage and clear the overflow flag.
        if (!transfer->rx_active) {
            while (s->SPI.INTFLAG.bit.RXC == 1) {
                s->SPI.DATA.reg;
            }
            s->SPI.STATUS.bit.BUFOVF = 1;
            s->SPI.INTFLAG.reg = SERCOM_SPI_INTFLAG_ERROR;
        }
    }

    return true;
}

int shared_dma_transfer_close(dma_transfer_t* transfer) {
    // It is not an error if either is NO_DMA_CHANNEL.
    dma_free_channel(transfer->tx_channel);
    dma_free_channel(transfer->rx_channel);

    if (transfer->failure != 0) {
        return transfer->failure;
    }

    if ((!transfer->rx_active || dma_transfer_status(transfer->rx_channel) == DMAC_CHINTFLAG_TCMPL) &&
        (!transfer->tx_active || dma_transfer_status(transfer->tx_channel) == DMAC_CHINTFLAG_TCMPL)) {
        return transfer->length;
    }
    return DMA_FAILURE_INCOMPLETE;
}

// Do write and read simultaneously. If buffer_out is NULL, write the tx byte over and over.
// If buffer_out is a real buffer, ignore tx.
// DMAs buffer_out -> dest
// DMAs src -> buffer_in
static int32_t shared_dma_transfer(void* peripheral,
                                   const uint8_t* buffer_out, volatile uint32_t* dest,
                                   volatile uint32_t* src, uint8_t* buffer_in,
                                   uint32_t length, uint8_t tx) {
    dma_transfer_t transfer;
    shared_dma_transfer_start(&transfer, peripheral, buffer_out, dest, src, buffer_in, length, tx);
    if (transfer.failure != 0) {
        return transfer.failure;
    }
    while (!shared_dma_transfer_finished(&transfer)) {}

    return shared_dma_transfer_close(&transfer);
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
