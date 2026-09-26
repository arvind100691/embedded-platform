# ============================================================================
# STM32F103 integration-test firmware
#
# This directory MUST NOT add the STM32F103 platform or BSP with
# add_subdirectory(). Those targets are created by the root CMakeLists.txt.
# ============================================================================

# ============================================================================
# Startup file
# ============================================================================

set(STM32F103_STARTUP_FILE
    ${CMAKE_SOURCE_DIR}/external/stm32cube-f1/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s
)

if(NOT EXISTS "${STM32F103_STARTUP_FILE}")

    message(FATAL_ERROR "STM32F103 GCC startup file not found: " "${STM32F103_STARTUP_FILE}")

endif()

# ============================================================================
# Linker script
# ============================================================================

set(STM32F103_LINKER_SCRIPT
    ${CMAKE_SOURCE_DIR}/platform/ports/stm32/stm32f103/startup/STM32F103C8Tx_FLASH.ld
)

if(NOT EXISTS "${STM32F103_LINKER_SCRIPT}")

    message(FATAL_ERROR "STM32F103 linker script not found: " "${STM32F103_LINKER_SCRIPT}")

endif()

# ============================================================================
# Integration-test executable
# ============================================================================

add_executable(
    stm32f103_gpio_integration_test
    GpioIntegrationTest.cpp ${CMAKE_SOURCE_DIR}/platform/bsp/stm32f103_board/src/stm32f1xx_it.cpp
    ${STM32F103_STARTUP_FILE}
)

# Explicit ELF extension.
set_target_properties(
    stm32f103_gpio_integration_test PROPERTIES OUTPUT_NAME "stm32f103_gpio_integration_test" SUFFIX
                                                                                             ".elf"
)

# ============================================================================
# Include paths / dependencies
# ============================================================================

target_include_directories(
    stm32f103_gpio_integration_test
    PRIVATE ${CMAKE_SOURCE_DIR}/platform/ports/stm32/stm32f103/include
            ${CMAKE_SOURCE_DIR}/platform/bsp/stm32f103_board/include
)

target_link_libraries(
    stm32f103_gpio_integration_test PRIVATE platform_common platform_hal platform_stm32f103
                                            bsp_stm32f103_board
)

target_compile_features(stm32f103_gpio_integration_test PRIVATE cxx_std_17)

# ============================================================================
# Compiler options
# ============================================================================

target_compile_options(
    stm32f103_gpio_integration_test
    PRIVATE -mcpu=cortex-m3
            -mthumb
            -ffunction-sections
            -fdata-sections
            -fno-exceptions
            -fno-rtti
            -fno-use-cxa-atexit
            -fno-threadsafe-statics
)

# ============================================================================
# Linker options
# ============================================================================

target_link_options(
    stm32f103_gpio_integration_test
    PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -T${STM32F103_LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-u,EXTI0_IRQHandler
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/stm32f103_gpio_integration_test.map
    --specs=nano.specs
    --specs=nosys.specs
)

# ============================================================================
# Post-build firmware artifacts
# ============================================================================

add_custom_command(
    TARGET stm32f103_gpio_integration_test
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:stm32f103_gpio_integration_test>
            ${CMAKE_CURRENT_BINARY_DIR}/stm32f103_gpio_integration_test.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stm32f103_gpio_integration_test>
            ${CMAKE_CURRENT_BINARY_DIR}/stm32f103_gpio_integration_test.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:stm32f103_gpio_integration_test>
    COMMENT "Generating STM32F103 GPIO integration-test firmware"
)
