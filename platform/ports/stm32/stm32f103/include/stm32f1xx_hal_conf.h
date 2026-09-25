#pragma once

/*
 * STM32F1 HAL configuration for the STM32F103 platform port.
 *
 * This file intentionally lives inside the MCU port and must not be
 * included by platform-independent code.
 */

/* ============================================================================
 * HAL modules
 * ========================================================================== */

#define HAL_MODULE_ENABLED

#define HAL_RCC_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED

/* Enable these when their corresponding platform implementations are added. */
/* #define HAL_UART_MODULE_ENABLED */
/* #define HAL_SPI_MODULE_ENABLED */
/* #define HAL_I2C_MODULE_ENABLED */
/* #define HAL_CAN_MODULE_ENABLED */
/* #define HAL_ADC_MODULE_ENABLED */
/* #define HAL_TIM_MODULE_ENABLED */
/* #define HAL_IWDG_MODULE_ENABLED */


/* ============================================================================
 * Oscillator configuration
 * ========================================================================== */

/*
 * External crystal frequency.
 *
 * Example board:
 * STM32F103C8T6
 * HSE = 8 MHz
 */
#if !defined(HSE_VALUE)
#define HSE_VALUE 8000000U
#endif

/*
 * HSE startup timeout in milliseconds.
 */
#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT 100U
#endif

/*
 * Internal high-speed oscillator.
 */
#if !defined(HSI_VALUE)
#define HSI_VALUE 8000000U
#endif

/*
 * Internal low-speed oscillator.
 */
#if !defined(LSI_VALUE)
#define LSI_VALUE 40000U
#endif

/*
 * External low-speed oscillator.
 *
 * We are not using LSE in the current board implementation,
 * but the HAL expects the configuration macro to exist.
 */
#if !defined(LSE_VALUE)
#define LSE_VALUE 32768U
#endif

#if !defined(LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT 5000U
#endif


/* ============================================================================
 * System configuration
 * ========================================================================== */

#define VDD_VALUE 3300U

/*
 * SysTick interrupt priority.
 *
 * We will revisit this when the RTOS layer is introduced.
 */
#define TICK_INT_PRIORITY 0U

#define USE_RTOS 0U

#define PREFETCH_ENABLE 1U


/* ============================================================================
 * HAL assertions
 * ========================================================================== */

/*
 * Keep HAL parameter assertions disabled for now.
 *
 * During platform development we can later enable:
 *
 *     #define USE_FULL_ASSERT 1U
 *
 * and provide assert_failed().
 */

#ifdef USE_FULL_ASSERT

#define assert_param(expr) \
    ((expr) ? (void)0U : \
     assert_failed((uint8_t *)__FILE__, __LINE__))

void assert_failed(
    uint8_t* file,
    uint32_t line);

#else

#define assert_param(expr) \
    ((void)0U)

#endif


/* ============================================================================
 * HAL register callback configuration
 * ========================================================================== */

#define USE_HAL_ADC_REGISTER_CALLBACKS       0U
#define USE_HAL_CAN_REGISTER_CALLBACKS       0U
#define USE_HAL_I2C_REGISTER_CALLBACKS       0U
#define USE_HAL_I2S_REGISTER_CALLBACKS       0U
#define USE_HAL_IRDA_REGISTER_CALLBACKS      0U
#define USE_HAL_PCCARD_REGISTER_CALLBACKS    0U
#define USE_HAL_PCD_REGISTER_CALLBACKS       0U
#define USE_HAL_RTC_REGISTER_CALLBACKS       0U
#define USE_HAL_SMARTCARD_REGISTER_CALLBACKS 0U
#define USE_HAL_SPI_REGISTER_CALLBACKS       0U
#define USE_HAL_SRAM_REGISTER_CALLBACKS      0U
#define USE_HAL_TIM_REGISTER_CALLBACKS       0U
#define USE_HAL_UART_REGISTER_CALLBACKS      0U
#define USE_HAL_USART_REGISTER_CALLBACKS     0U
#define USE_HAL_WWDG_REGISTER_CALLBACKS      0U


/* ============================================================================
 * HAL peripheral headers
 * ========================================================================== */

#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32f1xx_hal_cortex.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32f1xx_hal_flash.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32f1xx_hal_gpio.h"
#endif

#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32f1xx_hal_rcc.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
#include "stm32f1xx_hal_uart.h"
#endif

#ifdef HAL_USART_MODULE_ENABLED
#include "stm32f1xx_hal_usart.h"
#endif

#ifdef HAL_SPI_MODULE_ENABLED
#include "stm32f1xx_hal_spi.h"
#endif

#ifdef HAL_I2C_MODULE_ENABLED
#include "stm32f1xx_hal_i2c.h"
#endif

#ifdef HAL_CAN_MODULE_ENABLED
#include "stm32f1xx_hal_can.h"
#endif

#ifdef HAL_ADC_MODULE_ENABLED
#include "stm32f1xx_hal_adc.h"
#endif

#ifdef HAL_TIM_MODULE_ENABLED
#include "stm32f1xx_hal_tim.h"
#endif

#ifdef HAL_IWDG_MODULE_ENABLED
#include "stm32f1xx_hal_iwdg.h"
#endif