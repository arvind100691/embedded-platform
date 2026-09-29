#include "stm32f1xx_hal.h"

extern "C" void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{
    if (uartHandle == nullptr)
    {
        return;
    }

    if (uartHandle->Instance == USART1)
    {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitTypeDef gpioInit{};

        /* USART1 TX: PA9 */
        gpioInit.Pin = GPIO_PIN_9;
        gpioInit.Mode = GPIO_MODE_AF_PP;
        gpioInit.Pull = GPIO_NOPULL;
        gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &gpioInit);

        /* USART1 RX: PA10 */
        gpioInit.Pin = GPIO_PIN_10;
        gpioInit.Mode = GPIO_MODE_INPUT;
        gpioInit.Pull = GPIO_NOPULL;
        gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &gpioInit);

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

    if (uartHandle->Instance == USART1)
    {
        HAL_NVIC_DisableIRQ(USART1_IRQn);

        __HAL_RCC_USART1_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);
    }
}
