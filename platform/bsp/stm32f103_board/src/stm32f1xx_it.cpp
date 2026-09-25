#include "stm32f1xx_hal.h"

extern "C"
{

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}

}