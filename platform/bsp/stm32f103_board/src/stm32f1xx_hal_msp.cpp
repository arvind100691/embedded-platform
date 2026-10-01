#include "platform/bsp/stm32f103_board/BoardConfig.hpp"

#include "stm32f1xx_hal.h"

namespace platform::bsp::stm32f103_board
{

void ensureHalMspLinked() {}

} // namespace platform::bsp::stm32f103_board

extern "C" void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{
    if (uartHandle == nullptr)
    {
        return;
    }

    if (uartHandle->Instance == platform::bsp::stm32f103_board::BoardConfig::consoleUartInstance())
    {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitTypeDef gpioInit{};

        gpioInit.Pin = platform::bsp::stm32f103_board::BoardConfig::consoleTxPin();
        gpioInit.Mode = GPIO_MODE_AF_PP;
        gpioInit.Pull = GPIO_NOPULL;
        gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(platform::bsp::stm32f103_board::BoardConfig::consoleTxPort(), &gpioInit);

        gpioInit.Pin = platform::bsp::stm32f103_board::BoardConfig::consoleRxPin();
        gpioInit.Mode = GPIO_MODE_INPUT;
        gpioInit.Pull = GPIO_NOPULL;
        gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(platform::bsp::stm32f103_board::BoardConfig::consoleRxPort(), &gpioInit);

        HAL_NVIC_SetPriority(USART1_IRQn, 5U, 0U);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
}

extern "C" void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{
    if (uartHandle == nullptr)
    {
        return;
    }

    if (uartHandle->Instance == platform::bsp::stm32f103_board::BoardConfig::consoleUartInstance())
    {
        HAL_NVIC_DisableIRQ(USART1_IRQn);

        __HAL_RCC_USART1_CLK_DISABLE();

        HAL_GPIO_DeInit(platform::bsp::stm32f103_board::BoardConfig::consoleTxPort(),
                        platform::bsp::stm32f103_board::BoardConfig::consoleTxPin() |
                            platform::bsp::stm32f103_board::BoardConfig::consoleRxPin());
    }
}
