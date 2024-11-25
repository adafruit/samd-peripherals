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

#ifndef MICROPY_INCLUDED_ATMEL_SAMD_PERIPHERALS_DMA_H
#define MICROPY_INCLUDED_ATMEL_SAMD_PERIPHERALS_DMA_H

#include <stdbool.h>
#include <stdint.h>

#include "include/sam.h"

// We allocate DMA resources for the entire lifecycle of the board (not the
// vm) because the general_dma resource will be shared between the REPL and SPI
// flash. Both uses must block each other in order to prevent conflict.
#define AUDIO_DMA_CHANNEL_COUNT 4
#define DMA_CHANNEL_COUNT 32

uint8_t dma_allocate_channel(bool audio_channel);
void dma_free_channel(uint8_t channel);

void init_shared_dma(void);

#ifdef SAM_D5X_E5X
int32_t qspi_dma_write(uint32_t address, const uint8_t* buffer, uint32_t length);
int32_t qspi_dma_read(uint32_t address, uint8_t* buffer, uint32_t length);
#endif

uint8_t sercom_index(Sercom* sercom);

int32_t sercom_dma_write(Sercom* sercom, const uint8_t* buffer, uint32_t length);
int32_t sercom_dma_read(Sercom* sercom, uint8_t* buffer, uint32_t length, uint8_t tx);
int32_t sercom_dma_transfer(Sercom* sercom, const uint8_t* buffer_out, uint8_t* buffer_in, uint32_t length);

typedef struct {
    void* peripheral;
    uint32_t length;
    uint8_t progress;
    uint8_t rx_channel;
    uint8_t tx_channel;
    bool rx_active;
    bool tx_active;
    bool sercom;
    int8_t failure;
} dma_descr_t;

dma_descr_t shared_dma_transfer_start(void* peripheral,
                                   const uint8_t* buffer_out, volatile uint32_t* dest,
                                   volatile uint32_t* src, uint8_t* buffer_in,
                                   uint32_t length, uint8_t tx);
bool shared_dma_transfer_finished(dma_descr_t descr);
int shared_dma_transfer_close(dma_descr_t descr);

void dma_configure(uint8_t channel_number, uint8_t trigsrc, bool output_event);
void dma_enable_channel(uint8_t channel_number);
void dma_disable_channel(uint8_t channel_number);
void dma_suspend_channel(uint8_t channel_number);
void dma_resume_channel(uint8_t channel_number);
bool dma_channel_free(uint8_t channel_number);
bool dma_channel_enabled(uint8_t channel_number);
uint8_t dma_transfer_status(uint8_t channel_number);
DmacDescriptor* dma_descriptor(uint8_t channel_number);
DmacDescriptor* dma_write_back_descriptor(uint8_t channel_number);

#endif  // MICROPY_INCLUDED_ATMEL_SAMD_PERIPHERALS_DMA_H
