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

# ============================================================================
# STM32F103 asynchronous UART integration-test firmware
#
# Hardware loopback:
#     PA9 (USART1 TX) <-> PA10 (USART1 RX)
# ============================================================================

add_executable(
    stm32f103_async_uart_integration_test
    UartAsyncIntegrationTest.cpp
    ${STM32F103_STARTUP_FILE}
    ${CMAKE_SOURCE_DIR}/platform/ports/stm32/stm32f103/src/syscalls.c
)

set_target_properties(
    stm32f103_async_uart_integration_test
    PROPERTIES OUTPUT_NAME "stm32f103_async_uart_integration_test" SUFFIX ".elf"
)

target_include_directories(
    stm32f103_async_uart_integration_test
    PRIVATE ${CMAKE_SOURCE_DIR}/platform/ports/stm32/stm32f103/include
            ${CMAKE_SOURCE_DIR}/platform/bsp/stm32f103_board/include
)

target_link_libraries(
    stm32f103_async_uart_integration_test PRIVATE platform_common platform_hal platform_stm32f103
                                                  bsp_stm32f103_board
)

target_compile_features(stm32f103_async_uart_integration_test PRIVATE cxx_std_17)

stm32f103_configure_executable(stm32f103_async_uart_integration_test)

stm32f103_add_firmware_artifacts(stm32f103_async_uart_integration_test)

# ============================================================================
# STM32F103 buffered asynchronous UART integration-test firmware
#
# Hardware loopback:
#     PA9 (USART1 TX) <-> PA10 (USART1 RX)
# ============================================================================

add_executable(
    stm32f103_buffered_async_uart_integration_test
    BufferedAsyncUartIntegrationTest.cpp
    ${STM32F103_STARTUP_FILE}
    ${CMAKE_SOURCE_DIR}/platform/ports/stm32/stm32f103/src/syscalls.c
)

set_target_properties(
    stm32f103_buffered_async_uart_integration_test
    PROPERTIES OUTPUT_NAME "stm32f103_buffered_async_uart_integration_test" SUFFIX ".elf"
)

target_include_directories(
    stm32f103_buffered_async_uart_integration_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/platform/ports/stm32/stm32f103/include
        ${CMAKE_SOURCE_DIR}/platform/bsp/stm32f103_board/include
)

target_link_libraries(
    stm32f103_buffered_async_uart_integration_test
    PRIVATE
        platform_common
        platform_hal
        platform_stm32f103
        platform_communication
        bsp_stm32f103_board
)

target_compile_features(stm32f103_buffered_async_uart_integration_test PRIVATE cxx_std_17)

stm32f103_configure_executable(stm32f103_buffered_async_uart_integration_test)

stm32f103_add_firmware_artifacts(stm32f103_buffered_async_uart_integration_test)
