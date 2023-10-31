
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

DMA_HandleTypeDef handle_GPDMA1_Channel7{};
DMA_HandleTypeDef handle_GPDMA2_Channel7{};

extern "C" __attribute__((used)) void GPDMA1_Channel7_IRQHandler(void) { HAL_DMA_IRQHandler(&handle_GPDMA1_Channel7); }

extern "C" __attribute__((used)) void GPDMA2_Channel7_IRQHandler(void) { HAL_DMA_IRQHandler(&handle_GPDMA2_Channel7); }

static void MX_GPDMA1_Init(void) {
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    HAL_NVIC_SetPriority(GPDMA1_Channel7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel7_IRQn);
}

/**
 * @brief GPDMA2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPDMA2_Init(void) {
    __HAL_RCC_GPDMA2_CLK_ENABLE();

    HAL_NVIC_SetPriority(GPDMA2_Channel7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA2_Channel7_IRQn);
}

SPI_HandleTypeDef hspi1{};
SPI_HandleTypeDef hspi2{};

extern "C" __attribute__((used)) void SPI1_IRQHandler(void) { HAL_SPI_IRQHandler(&hspi1); }

extern "C" __attribute__((used)) void SPI2_IRQHandler(void) { HAL_SPI_IRQHandler(&hspi2); }

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {  // cppcheck-suppress constParameterPointer
    GPIO_InitTypeDef GPIO_InitStruct{};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct{};
    if (hspi->Instance == SPI1) {
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI1;
        PeriphClkInitStruct.PLL2.PLL2Source = RCC_PLL2_SOURCE_HSE;
        PeriphClkInitStruct.PLL2.PLL2M = 1;
        PeriphClkInitStruct.PLL2.PLL2N = 30;
        PeriphClkInitStruct.PLL2.PLL2P = 30;
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

        handle_GPDMA1_Channel7.Instance = GPDMA1_Channel7;
        handle_GPDMA1_Channel7.Init.Request = GPDMA1_REQUEST_SPI1_TX;
        handle_GPDMA1_Channel7.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        handle_GPDMA1_Channel7.Init.Direction = DMA_MEMORY_TO_PERIPH;
        handle_GPDMA1_Channel7.Init.SrcInc = DMA_SINC_INCREMENTED;
        handle_GPDMA1_Channel7.Init.DestInc = DMA_DINC_FIXED;
        handle_GPDMA1_Channel7.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        handle_GPDMA1_Channel7.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
        handle_GPDMA1_Channel7.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        handle_GPDMA1_Channel7.Init.SrcBurstLength = 1;
        handle_GPDMA1_Channel7.Init.DestBurstLength = 1;
        handle_GPDMA1_Channel7.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
        handle_GPDMA1_Channel7.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        handle_GPDMA1_Channel7.Init.Mode = DMA_NORMAL;
        if (HAL_DMA_Init(&handle_GPDMA1_Channel7) != HAL_OK) {
            while (1) {
            }
        }

        __HAL_LINKDMA(hspi, hdmatx, handle_GPDMA1_Channel7);

        if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel7, DMA_CHANNEL_NPRIV) != HAL_OK) {
            while (1) {
            }
        }

        HAL_NVIC_SetPriority(SPI1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(SPI1_IRQn);

    } else if (hspi->Instance == SPI2) {
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI2;
        PeriphClkInitStruct.PLL3.PLL3Source = RCC_PLL3_SOURCE_HSE;
        PeriphClkInitStruct.PLL3.PLL3M = 1;
        PeriphClkInitStruct.PLL3.PLL3N = 30;
        PeriphClkInitStruct.PLL3.PLL3P = 30;
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

        handle_GPDMA2_Channel7.Instance = GPDMA2_Channel7;
        handle_GPDMA2_Channel7.Init.Request = GPDMA1_REQUEST_SPI2_TX;
        handle_GPDMA2_Channel7.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        handle_GPDMA2_Channel7.Init.Direction = DMA_MEMORY_TO_PERIPH;
        handle_GPDMA2_Channel7.Init.SrcInc = DMA_SINC_INCREMENTED;
        handle_GPDMA2_Channel7.Init.DestInc = DMA_DINC_FIXED;
        handle_GPDMA2_Channel7.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        handle_GPDMA2_Channel7.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
        handle_GPDMA2_Channel7.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        handle_GPDMA2_Channel7.Init.SrcBurstLength = 1;
        handle_GPDMA2_Channel7.Init.DestBurstLength = 1;
        handle_GPDMA2_Channel7.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
        handle_GPDMA2_Channel7.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        handle_GPDMA2_Channel7.Init.Mode = DMA_NORMAL;
        if (HAL_DMA_Init(&handle_GPDMA2_Channel7) != HAL_OK) {
            while (1) {
            }
        }

        __HAL_LINKDMA(hspi, hdmatx, handle_GPDMA2_Channel7);

        if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA2_Channel7, DMA_CHANNEL_NPRIV) != HAL_OK) {
            while (1) {
            }
        }

        HAL_NVIC_SetPriority(SPI2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(SPI2_IRQn);
    }
}

static void MX_SPI1_Init(void) {
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 0x7;
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
    hspi1.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
    hspi1.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        while (1) {
        }
    }
}

static void MX_SPI2_Init(void) {
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 0x7;
    hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
    hspi2.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
    hspi2.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
    if (HAL_SPI_Init(&hspi2) != HAL_OK) {
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

void SPI_0::setupDMATransfer() {
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

bool SPI_0::isDMAbusy() const {
    //    HAL_DMA_PollForTransfer(&handle_GPDMA1_Channel7, HAL_DMA_FULL_TRANSFER, 0);
    //    if (handle_GPDMA1_Channel7.State == HAL_DMA_STATE_BUSY) {
    //        return true;
    //    }
    return false;
}

void SPI_0::init() {
    MX_GPDMA1_Init();
    MX_SPI1_Init();
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

void SPI_1::setupDMATransfer() {
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

bool SPI_1::isDMAbusy() const {
    //    HAL_DMA_PollForTransfer(&handle_GPDMA2_Channel7, HAL_DMA_FULL_TRANSFER, 0);
    //    if (handle_GPDMA2_Channel7.State == HAL_DMA_STATE_BUSY) {
    //        return true;
    //    }
    return false;
}

void SPI_1::init() {
    MX_GPDMA2_Init();
    MX_SPI2_Init();
    HAL_DMA_RegisterCallback(&handle_GPDMA2_Channel7, HAL_DMA_XFER_CPLT_CB_ID, &SPI2_IT_Callback);
}
