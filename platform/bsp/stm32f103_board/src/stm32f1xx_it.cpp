#include "platform/stm32f103/Stm32Uart.hpp"

#include "stm32f1xx_hal.h"

extern "C" void SysTick_Handler(void)
{
    HAL_IncTick();
}

extern "C" void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

extern "C" void USART1_IRQHandler(void)
{
    platform::stm32f103::Stm32Uart::irqHandler(USART1);
}
