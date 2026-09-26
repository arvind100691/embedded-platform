# ============================================================================
# STM32CubeF1 paths
# ============================================================================

set(STM32CUBE_F1_DIR ${CMAKE_SOURCE_DIR}/external/stm32cube-f1)

set(STM32F1_HAL_DIR ${STM32CUBE_F1_DIR}/Drivers/STM32F1xx_HAL_Driver)

set(STM32F1_CMSIS_DIR ${STM32CUBE_F1_DIR}/Drivers/CMSIS)

set(STM32F1_DEVICE_DIR ${STM32F1_CMSIS_DIR}/Device/ST/STM32F1xx)

# ============================================================================
# STM32F103 startup and system files
# ============================================================================

set(STM32F103_STARTUP_FILE ${STM32F1_DEVICE_DIR}/Source/Templates/gcc/startup_stm32f103xb.s)

set(STM32F1_SYSTEM_FILE ${STM32F1_DEVICE_DIR}/Source/Templates/system_stm32f1xx.c)

# ============================================================================
# Validate external dependency
# ============================================================================

if(NOT EXISTS "${STM32CUBE_F1_DIR}")

    message(FATAL_ERROR "STM32CubeF1 dependency not found: " "${STM32CUBE_F1_DIR}")

endif()

if(NOT EXISTS "${STM32F103_STARTUP_FILE}")

    message(FATAL_ERROR "STM32F103 GCC startup file not found: " "${STM32F103_STARTUP_FILE}")

endif()

if(NOT EXISTS "${STM32F1_SYSTEM_FILE}")

    message(FATAL_ERROR "STM32F1 system file not found: " "${STM32F1_SYSTEM_FILE}")

endif()

# ============================================================================
# STM32 HAL vendor library
# ============================================================================

add_library(
    stm32f1xx_hal STATIC
    ${STM32F1_HAL_DIR}/Src/stm32f1xx_hal.c
    ${STM32F1_HAL_DIR}/Src/stm32f1xx_hal_cortex.c
    ${STM32F1_HAL_DIR}/Src/stm32f1xx_hal_gpio.c
    ${STM32F1_HAL_DIR}/Src/stm32f1xx_hal_rcc.c
    ${STM32F1_HAL_DIR}/Src/stm32f1xx_hal_rcc_ex.c
    ${STM32F1_HAL_DIR}/Src/stm32f1xx_hal_flash.c
    ${STM32F1_HAL_DIR}/Src/stm32f1xx_hal_flash_ex.c
    ${STM32F1_SYSTEM_FILE}
)

target_include_directories(
    stm32f1xx_hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${STM32F1_HAL_DIR}/Inc
                         ${STM32F1_CMSIS_DIR}/Core/Include ${STM32F1_DEVICE_DIR}/Include
)

target_compile_definitions(stm32f1xx_hal PUBLIC USE_HAL_DRIVER STM32F103xB)

target_compile_options(
    stm32f1xx_hal PUBLIC -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections
)

# ============================================================================
# Our STM32F103 platform implementation
# ============================================================================

add_library(platform_stm32f103 STATIC src/Stm32Gpio.cpp)

target_include_directories(platform_stm32f103 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(platform_stm32f103 PUBLIC platform_hal stm32f1xx_hal)

target_compile_features(platform_stm32f103 PUBLIC cxx_std_17)

target_compile_options(
    platform_stm32f103
    PRIVATE -mcpu=cortex-m3
            -mthumb
            -ffunction-sections
            -fdata-sections
            -fno-exceptions
            -fno-rtti
            -fno-use-cxa-atexit
            -fno-threadsafe-statics
)
