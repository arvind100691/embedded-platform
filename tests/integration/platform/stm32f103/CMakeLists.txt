# ============================================================================
# STM32F103 GPIO integration-test firmware
#
# The STM32F103 platform and BSP targets are created by the root
# CMakeLists.txt. This directory must not call add_subdirectory() for them.
# ============================================================================

# ============================================================================
# Integration-test executable
# ============================================================================

add_executable(
    stm32f103_gpio_integration_test
    GpioIntegrationTest.cpp
    # Explicitly include the interrupt vector handler object in this
    # executable. This is intentionally kept as part of the current working
    # solution for EXTI0_IRQHandler retention.
    ${CMAKE_SOURCE_DIR}/platform/bsp/stm32f103_board/src/stm32f1xx_it.cpp
    # STM32 startup code
    ${STM32F103_STARTUP_FILE}
    # Newlib syscall support
    ${CMAKE_SOURCE_DIR}/platform/ports/stm32/stm32f103/src/syscalls.c
)

set_target_properties(
    stm32f103_gpio_integration_test PROPERTIES OUTPUT_NAME "stm32f103_gpio_integration_test" SUFFIX
                                                                                             ".elf"
)

# ============================================================================
# Integration-test include paths
# ============================================================================

target_include_directories(
    stm32f103_gpio_integration_test
    PRIVATE ${CMAKE_SOURCE_DIR}/platform/ports/stm32/stm32f103/include
            ${CMAKE_SOURCE_DIR}/platform/bsp/stm32f103_board/include
)

# ============================================================================
# Dependencies
# ============================================================================

target_link_libraries(
    stm32f103_gpio_integration_test PRIVATE platform_common platform_hal platform_stm32f103
                                            bsp_stm32f103_board
)

target_compile_features(stm32f103_gpio_integration_test PRIVATE cxx_std_17)

# ============================================================================
# STM32F103 compiler + linker configuration
# ============================================================================

stm32f103_configure_executable(stm32f103_gpio_integration_test)

# ============================================================================
# Interrupt handler retention
#
# The integration test requires EXTI0_IRQHandler to survive
# --gc-sections because the startup vector contains a weak default handler.
# ============================================================================

target_link_options(stm32f103_gpio_integration_test PRIVATE -Wl,-u,EXTI0_IRQHandler)

# ============================================================================
# HEX / BIN generation
# ============================================================================

stm32f103_add_firmware_artifacts(stm32f103_gpio_integration_test)

# ============================================================================
# STM32F103 UART integration-test firmware
#
# Hardware loopback required:
#     PA9 (USART1 TX) <-> PA10 (USART1 RX)
# ============================================================================

add_executable(
    stm32f103_uart_integration_test
    UartIntegrationTest.cpp
    # The BSP MSP implementation contains the strong HAL UART MSP hook.
    # Include it directly here for the same static-library/weak-symbol reason
    # encountered previously with the GPIO EXTI handler.
    ${CMAKE_SOURCE_DIR}/platform/bsp/stm32f103_board/src/stm32f1xx_it.cpp
    ${CMAKE_SOURCE_DIR}/platform/bsp/stm32f103_board/src/stm32f1xx_hal_msp.cpp
    ${STM32F103_STARTUP_FILE}
    ${CMAKE_SOURCE_DIR}/platform/ports/stm32/stm32f103/src/syscalls.c
)

set_target_properties(
    stm32f103_uart_integration_test PROPERTIES OUTPUT_NAME "stm32f103_uart_integration_test" SUFFIX
                                                                                             ".elf"
)

target_include_directories(
    stm32f103_uart_integration_test
    PRIVATE ${CMAKE_SOURCE_DIR}/platform/ports/stm32/stm32f103/include
            ${CMAKE_SOURCE_DIR}/platform/bsp/stm32f103_board/include
)

target_link_libraries(
    stm32f103_uart_integration_test PRIVATE platform_common platform_hal platform_stm32f103
                                            bsp_stm32f103_board
)

target_compile_features(stm32f103_uart_integration_test PRIVATE cxx_std_17)

stm32f103_configure_executable(stm32f103_uart_integration_test)

stm32f103_add_firmware_artifacts(stm32f103_uart_integration_test)
