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
#include "./pwmtimer.h"

#include <stdio.h>

#include "stm32h5xx_hal.h"

static TIM_HandleTypeDef htim2{};
static TIM_HandleTypeDef htim3{};
static TIM_HandleTypeDef htim4{};
static TIM_HandleTypeDef htim15{};
static TIM_HandleTypeDef htim16{};
static TIM_HandleTypeDef htim17{};

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim_pwm) {  // cppcheck-suppress constParameterPointer
    if (htim_pwm->Instance == TIM2) {
        __HAL_RCC_TIM2_CLK_ENABLE();
    } else if (htim_pwm->Instance == TIM3) {
        __HAL_RCC_TIM3_CLK_ENABLE();
    } else if (htim_pwm->Instance == TIM4) {
        __HAL_RCC_TIM4_CLK_ENABLE();
    } else if (htim_pwm->Instance == TIM15) {
        __HAL_RCC_TIM15_CLK_ENABLE();
    }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim_base) {  // cppcheck-suppress constParameterPointer
    if (htim_base->Instance == TIM16) {
        __HAL_RCC_TIM16_CLK_ENABLE();
    } else if (htim_base->Instance == TIM17) {
        __HAL_RCC_TIM17_CLK_ENABLE();
    }
}

static void HAL_TIM_MspPostInit(const TIM_HandleTypeDef *htim) {
    GPIO_InitTypeDef GPIO_InitStruct{};
    if (htim->Instance == TIM2) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**TIM2 GPIO Configuration
        PA0     ------> TIM2_CH1
        */
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    } else if (htim->Instance == TIM3) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**TIM3 GPIO Configuration
        PB4(NJTRST)     ------> TIM3_CH1
        */
        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    } else if (htim->Instance == TIM4) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**TIM4 GPIO Configuration
        PB6     ------> TIM4_CH1
        */
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    } else if (htim->Instance == TIM15) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
        /**TIM15 GPIO Configuration
        PC12     ------> TIM15_CH1
        */
        GPIO_InitStruct.Pin = GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM15;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    } else if (htim->Instance == TIM16) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**TIM16 GPIO Configuration
        PB8     ------> TIM16_CH1
        */
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM16;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    } else if (htim->Instance == TIM17) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**TIM17 GPIO Configuration
        PB9     ------> TIM17_CH1
        */
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM17;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

static void MX_TIM2_Init(void) {
    TIM_MasterConfigTypeDef sMasterConfig{};
    TIM_OC_InitTypeDef sConfigOC{};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = PwmTimer::pwmPeriod;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        while (1) {
        }
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) {
        while (1) {
        }
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = PwmTimer::initPulse;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        while (1) {
        }
    }
    HAL_TIM_MspPostInit(&htim2);
}

static void MX_TIM3_Init(void) {
    TIM_MasterConfigTypeDef sMasterConfig = {};
    TIM_OC_InitTypeDef sConfigOC{};

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 0;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = PwmTimer::pwmPeriod;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        while (1) {
        }
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
        while (1) {
        }
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = PwmTimer::initPulse;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        while (1) {
        }
    }
    HAL_TIM_MspPostInit(&htim3);
}

static void MX_TIM4_Init(void) {
    TIM_MasterConfigTypeDef sMasterConfig{};
    TIM_OC_InitTypeDef sConfigOC{};

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 0;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = PwmTimer::pwmPeriod;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim4) != HAL_OK) {
        while (1) {
        }
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK) {
        while (1) {
        }
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = PwmTimer::initPulse;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        while (1) {
        }
    }
    HAL_TIM_MspPostInit(&htim4);
}

static void MX_TIM15_Init(void) {
    TIM_MasterConfigTypeDef sMasterConfig{};
    TIM_OC_InitTypeDef sConfigOC{};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig{};

    htim15.Instance = TIM15;
    htim15.Init.Prescaler = 0;
    htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim15.Init.Period = PwmTimer::pwmPeriod;
    htim15.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim15.Init.RepetitionCounter = 0;
    htim15.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim15) != HAL_OK) {
        while (1) {
        }
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim15, &sMasterConfig) != HAL_OK) {
        while (1) {
        }
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = PwmTimer::initPulse;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim15, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        while (1) {
        }
    }
    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.BreakFilter = 0;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim15, &sBreakDeadTimeConfig) != HAL_OK) {
        while (1) {
        }
    }
    HAL_TIM_MspPostInit(&htim15);
}

static void MX_TIM16_Init(void) {
    TIM_OC_InitTypeDef sConfigOC{};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig{};

    htim16.Instance = TIM16;
    htim16.Init.Prescaler = 0;
    htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim16.Init.Period = PwmTimer::pwmPeriod;
    htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim16.Init.RepetitionCounter = 0;
    htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim16) != HAL_OK) {
        while (1) {
        }
    }
    if (HAL_TIM_PWM_Init(&htim16) != HAL_OK) {
        while (1) {
        }
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = PwmTimer::initPulse;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim16, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        while (1) {
        }
    }
    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.BreakFilter = 0;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim16, &sBreakDeadTimeConfig) != HAL_OK) {
        while (1) {
        }
    }
    HAL_TIM_MspPostInit(&htim16);
}

static void MX_TIM17_Init(void) {
    TIM_OC_InitTypeDef sConfigOC{};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig{};

    htim17.Instance = TIM17;
    htim17.Init.Prescaler = 0;
    htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim17.Init.Period = PwmTimer::pwmPeriod;
    htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim17.Init.RepetitionCounter = 0;
    htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim17) != HAL_OK) {
        while (1) {
        }
    }
    if (HAL_TIM_PWM_Init(&htim17) != HAL_OK) {
        while (1) {
        }
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = PwmTimer::initPulse;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim17, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        while (1) {
        }
    }
    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.BreakFilter = 0;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim17, &sBreakDeadTimeConfig) != HAL_OK) {
        while (1) {
        }
    }
    HAL_TIM_MspPostInit(&htim17);
}

PwmTimer &PwmTimer0::instance() {
    static PwmTimer0 timer;
    if (!timer.initialized) {
        timer.initialized = true;
        timer.init();
    }
    return timer;
}

void PwmTimer0::init() { MX_TIM2_Init(); }
void PwmTimer0::start() { HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); }
void PwmTimer0::stop() { HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1); }

void PwmTimer0::setPulse(uint16_t pulse) { TIM2->CCR1 = pulse; }

PwmTimer &PwmTimer1::instance() {
    static PwmTimer1 timer;
    if (!timer.initialized) {
        timer.initialized = true;
        timer.init();
    }
    return timer;
}

void PwmTimer1::init() { MX_TIM15_Init(); }
void PwmTimer1::start() { HAL_TIM_PWM_Start(&htim15, TIM_CHANNEL_1); }
void PwmTimer1::stop() { HAL_TIM_PWM_Stop(&htim15, TIM_CHANNEL_1); }

void PwmTimer1::setPulse(uint16_t pulse) { TIM15->CCR1 = pulse; }

PwmTimer &PwmTimer2::instance() {
    static PwmTimer2 timer;
    if (!timer.initialized) {
        timer.initialized = true;
        timer.init();
    }
    return timer;
}

void PwmTimer2::init() { MX_TIM3_Init(); }
void PwmTimer2::start() { HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); }
void PwmTimer2::stop() { HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1); }

void PwmTimer2::setPulse(uint16_t pulse) { TIM3->CCR1 = pulse; }

PwmTimer &PwmTimer3::instance() {
    static PwmTimer3 timer;
    if (!timer.initialized) {
        timer.initialized = true;
        timer.init();
    }
    return timer;
}

void PwmTimer3::init() { MX_TIM4_Init(); }
void PwmTimer3::start() { HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1); }
void PwmTimer3::stop() { HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1); }

void PwmTimer3::setPulse(uint16_t pulse) { TIM4->CCR1 = pulse; }

PwmTimer &PwmTimer4::instance() {
    static PwmTimer4 timer;
    if (!timer.initialized) {
        timer.initialized = true;
        timer.init();
    }
    return timer;
}

void PwmTimer4::init() { MX_TIM16_Init(); }
void PwmTimer4::start() { HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1); }
void PwmTimer4::stop() { HAL_TIM_PWM_Stop(&htim16, TIM_CHANNEL_1); }

void PwmTimer4::setPulse(uint16_t pulse) { TIM16->CCR1 = pulse; }

PwmTimer &PwmTimer5::instance() {
    static PwmTimer5 timer;
    if (!timer.initialized) {
        timer.initialized = true;
        timer.init();
    }
    return timer;
}

void PwmTimer5::init() { MX_TIM17_Init(); }
void PwmTimer5::start() { HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1); }
void PwmTimer5::stop() { HAL_TIM_PWM_Stop(&htim17, TIM_CHANNEL_1); }

void PwmTimer5::setPulse(uint16_t pulse) { TIM17->CCR1 = pulse; }
