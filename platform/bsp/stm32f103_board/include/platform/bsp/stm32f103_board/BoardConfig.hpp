#pragma once

#include "stm32f1xx_hal.h"

namespace platform::bsp::stm32f103_board
{

struct BoardConfig
{
    static constexpr GPIO_TypeDef* statusLedPort()
    {
        return GPIOC;
    }

    static constexpr uint16_t statusLedPin()
    {
        return GPIO_PIN_13;
    }

    static constexpr GPIO_TypeDef* consoleTxPort()
    {
        return GPIOA;
    }

    static constexpr uint16_t consoleTxPin()
    {
        return GPIO_PIN_9;
    }

    static constexpr GPIO_TypeDef* consoleRxPort()
    {
        return GPIOA;
    }

    static constexpr uint16_t consoleRxPin()
    {
        return GPIO_PIN_10;
    }

    static constexpr USART_TypeDef* consoleUartInstance()
    {
        return USART1;
    }

    static constexpr uint32_t systemClockHz()
    {
        return 72000000U;
    }

    static constexpr uint32_t externalOscillatorHz()
    {
        return 8000000U;
    }
};

} // namespace platform::bsp::stm32f103_board
