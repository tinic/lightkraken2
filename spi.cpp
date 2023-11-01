
/*
Copyright 2019 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "./spi.h"

#include <stdio.h>

#include <algorithm>

#include "./pwmtimer.h"
#include "stm32h5xx_hal.h"

static DMA_HandleTypeDef handle_GPDMA1_Channel7{};
static DMA_HandleTypeDef handle_GPDMA2_Channel7{};
static SPI_HandleTypeDef hspi1{};
static SPI_HandleTypeDef hspi2{};

extern "C" __attribute__((used)) void GPDMA1_Channel7_IRQHandler(void) { HAL_DMA_IRQHandler(&handle_GPDMA1_Channel7); }
extern "C" __attribute__((used)) void GPDMA2_Channel7_IRQHandler(void) { HAL_DMA_IRQHandler(&handle_GPDMA2_Channel7); }
extern "C" __attribute__((used)) void SPI1_IRQHandler(void) { HAL_SPI_IRQHandler(&hspi1); }
extern "C" __attribute__((used)) void SPI2_IRQHandler(void) { HAL_SPI_IRQHandler(&hspi2); }

static void configSPI1Clock(uint32_t mul = 30, uint32_t div = 30) {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct{};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI1;
    PeriphClkInitStruct.PLL2.PLL2Source = RCC_PLL2_SOURCE_HSE;
    PeriphClkInitStruct.PLL2.PLL2M = 1;
    PeriphClkInitStruct.PLL2.PLL2N = mul;
    PeriphClkInitStruct.PLL2.PLL2P = div;
    PeriphClkInitStruct.PLL2.PLL2Q = 2;
    PeriphClkInitStruct.PLL2.PLL2R = 2;
    PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2_VCIRANGE_3;
    PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2_VCORANGE_WIDE;
    PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
    PeriphClkInitStruct.PLL2.PLL2ClockOut = RCC_PLL2_DIVP;
    PeriphClkInitStruct.Spi1ClockSelection = RCC_SPI1CLKSOURCE_PLL2P;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        while (1) {
        }
    }
}

static void configSPI2Clock(uint32_t mul = 30, uint32_t div = 30) {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct{};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI2;
    PeriphClkInitStruct.PLL3.PLL3Source = RCC_PLL3_SOURCE_HSE;
    PeriphClkInitStruct.PLL3.PLL3M = 1;
    PeriphClkInitStruct.PLL3.PLL3N = mul;
    PeriphClkInitStruct.PLL3.PLL3P = div;
    PeriphClkInitStruct.PLL3.PLL3Q = 2;
    PeriphClkInitStruct.PLL3.PLL3R = 2;
    PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3_VCIRANGE_1;
    PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3_VCORANGE_WIDE;
    PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
    PeriphClkInitStruct.PLL3.PLL3ClockOut = RCC_PLL3_DIVP;
    PeriphClkInitStruct.Spi2ClockSelection = RCC_SPI2CLKSOURCE_PLL3P;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        while (1) {
        }
    }
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {  // cppcheck-suppress constParameterPointer
    GPIO_InitTypeDef GPIO_InitStruct{};

    auto dmaInit = [hspi](DMA_HandleTypeDef &handle, DMA_Channel_TypeDef *instance, uint32_t request) {
        handle.Instance = instance;
        handle.Init.Request = request;
        handle.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
        handle.Init.SrcInc = DMA_SINC_INCREMENTED;
        handle.Init.DestInc = DMA_DINC_FIXED;
        handle.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        handle.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
        handle.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        handle.Init.SrcBurstLength = 1;
        handle.Init.DestBurstLength = 1;
        handle.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
        handle.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        handle.Init.Mode = DMA_NORMAL;
        if (HAL_DMA_Init(&handle) != HAL_OK) {
            while (1) {
            }
        }

        __HAL_LINKDMA(hspi, hdmatx, handle);

        if (HAL_DMA_ConfigChannelAttributes(&handle, DMA_CHANNEL_NPRIV) != HAL_OK) {
            while (1) {
            }
        }
    };

    if (hspi->Instance == SPI1) {
        configSPI1Clock();

        __HAL_RCC_SPI1_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**SPI1 GPIO Configuration
        PA5     ------> SPI1_SCK
        PB5     ------> SPI1_MOSI
        */
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        dmaInit(handle_GPDMA1_Channel7, GPDMA1_Channel7, GPDMA1_REQUEST_SPI1_TX);

        HAL_NVIC_SetPriority(SPI1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(SPI1_IRQn);

    } else if (hspi->Instance == SPI2) {
        configSPI2Clock();

        __HAL_RCC_SPI2_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**SPI2 GPIO Configuration
        PC3     ------> SPI2_MOSI
        PB10     ------> SPI2_SCK
        */
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        dmaInit(handle_GPDMA2_Channel7, GPDMA2_Channel7, GPDMA1_REQUEST_SPI2_TX);

        HAL_NVIC_SetPriority(SPI2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(SPI2_IRQn);
    }
}

static void InitSPI(SPI_HandleTypeDef &handle, SPI_TypeDef *instance) {
    handle.Instance = instance;
    handle.Init.Mode = SPI_MODE_MASTER;
    handle.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
    handle.Init.DataSize = SPI_DATASIZE_8BIT;
    handle.Init.CLKPolarity = SPI_POLARITY_LOW;
    handle.Init.CLKPhase = SPI_PHASE_1EDGE;
    handle.Init.NSS = SPI_NSS_SOFT;
    handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    handle.Init.TIMode = SPI_TIMODE_DISABLE;
    handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    handle.Init.CRCPolynomial = 0x7;
    handle.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    handle.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    handle.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    handle.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    handle.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    handle.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    handle.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    handle.Init.IOSwap = SPI_IO_SWAP_DISABLE;
    handle.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
    handle.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
    if (HAL_SPI_Init(&handle) != HAL_OK) {
        while (1) {
        }
    }
}

SPI &SPI_0::instance() {
    static SPI_0 spi;
    if (!spi.initialized) {
        spi.initialized = true;
        spi.init();
    }
    return spi;
}

static void SPI1_IT_Callback(DMA_HandleTypeDef *) { SPI_0::instance().setDMAActive(false); }

void SPI_0::startDMATransfer() { HAL_SPI_Transmit_DMA(&hspi1, cbuf, uint16_t(clen)); }

void SPI_0::setupDMATransfer() { configSPI1Clock(mul, div); }

bool SPI_0::isDMAbusy() const { return ((handle_GPDMA1_Channel7.Instance->CSR & DMA_CSR_IDLEF) == 0); }

void SPI_0::init() {
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    HAL_NVIC_SetPriority(GPDMA1_Channel7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel7_IRQn);

    InitSPI(hspi1, SPI1);

    HAL_DMA_RegisterCallback(&handle_GPDMA1_Channel7, HAL_DMA_XFER_CPLT_CB_ID, &SPI1_IT_Callback);
}

SPI &SPI_1::instance() {
    static SPI_1 spi;
    if (!spi.initialized) {
        spi.initialized = true;
        spi.init();
    }
    return spi;
}

static void SPI2_IT_Callback(DMA_HandleTypeDef *) { SPI_1::instance().setDMAActive(false); }

void SPI_1::startDMATransfer() { HAL_SPI_Transmit_DMA(&hspi2, cbuf, uint16_t(clen)); }

void SPI_1::setupDMATransfer() { configSPI2Clock(mul, div); }

bool SPI_1::isDMAbusy() const { return ((handle_GPDMA2_Channel7.Instance->CSR & DMA_CSR_IDLEF) == 0); }

void SPI_1::init() {
    __HAL_RCC_GPDMA2_CLK_ENABLE();
    HAL_NVIC_SetPriority(GPDMA2_Channel7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA2_Channel7_IRQn);

    InitSPI(hspi2, SPI2);

    HAL_DMA_RegisterCallback(&handle_GPDMA2_Channel7, HAL_DMA_XFER_CPLT_CB_ID, &SPI2_IT_Callback);
}
