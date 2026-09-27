# ============================================================================
# STM32F1xx HAL vendor library
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

stm32f103_configure_target(stm32f1xx_hal)

# ============================================================================
# Project-owned STM32F103 platform implementation
# ============================================================================

add_library(platform_stm32f103 STATIC src/Stm32Gpio.cpp)

target_include_directories(platform_stm32f103 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(platform_stm32f103 PUBLIC platform_hal stm32f1xx_hal)

target_compile_features(platform_stm32f103 PUBLIC cxx_std_17)

stm32f103_configure_target(platform_stm32f103)
