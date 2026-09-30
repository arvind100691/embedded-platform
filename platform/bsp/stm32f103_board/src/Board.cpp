#include "platform/bsp/stm32f103_board/Board.hpp"

#include "platform/bsp/stm32f103_board/BoardConfig.hpp"
#include "platform/stm32f103/Stm32Gpio.hpp"
#include "platform/stm32f103/Stm32Uart.hpp"

#include "stm32f1xx_hal.h"

namespace
{

platform::stm32f103::Stm32Gpio
    g_statusLed(platform::bsp::stm32f103_board::BoardConfig::statusLedPort(),
                platform::bsp::stm32f103_board::BoardConfig::statusLedPin());
platform::stm32f103::Stm32Uart
    g_consoleUart(platform::bsp::stm32f103_board::BoardConfig::consoleUartInstance());

void SystemClock_Config()
{
    RCC_OscInitTypeDef oscConfig{};
    RCC_ClkInitTypeDef clockConfig{};

    oscConfig.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscConfig.HSEState = RCC_HSE_ON;
    oscConfig.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    oscConfig.HSIState = RCC_HSI_ON;
    oscConfig.PLL.PLLState = RCC_PLL_ON;
    oscConfig.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscConfig.PLL.PLLMUL = RCC_PLL_MUL9;

    static_cast<void>(platform::bsp::stm32f103_board::BoardConfig::externalOscillatorHz());
    static_cast<void>(platform::bsp::stm32f103_board::BoardConfig::systemClockHz());

    if (HAL_RCC_OscConfig(&oscConfig) != HAL_OK)
    {
        while (true)
        {
        }
    }

    clockConfig.ClockType =
        RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;

    clockConfig.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clockConfig.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clockConfig.APB1CLKDivider = RCC_HCLK_DIV2;
    clockConfig.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&clockConfig, FLASH_LATENCY_2) != HAL_OK)
    {
        while (true)
        {
        }
    }
}

} // namespace

namespace platform::bsp::stm32f103_board
{

void init()
{
    HAL_Init();

    SystemClock_Config();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
}

platform::hal::IGpio& statusLed()
{
    return g_statusLed;
}

platform::hal::IAsyncUart& consoleAsyncUart()
{
    return g_consoleUart;
}

platform::hal::IUart& consoleUart()
{
    return g_consoleUart;
}

} // namespace platform::bsp::stm32f103_board
