# ============================================================================
# STM32F103 build configuration
#
# Provides reusable MCU-specific build configuration for STM32F103 targets.
#
# Responsibilities:
#   - STM32CubeF1 paths
#   - Startup file
#   - Linker script
#   - ARM compiler flags
#   - Embedded C++ restrictions
#   - STM32 executable linker configuration
#   - HEX/BIN/MAP firmware artifacts
# ============================================================================

include_guard(GLOBAL)

# ============================================================================
# STM32CubeF1 paths
# ============================================================================

set(STM32CUBE_F1_DIR
    "${CMAKE_SOURCE_DIR}/external/stm32cube-f1"
    CACHE PATH "STM32CubeF1 root directory"
)

set(STM32F1_HAL_DIR "${STM32CUBE_F1_DIR}/Drivers/STM32F1xx_HAL_Driver")

set(STM32F1_CMSIS_DIR "${STM32CUBE_F1_DIR}/Drivers/CMSIS")

set(STM32F1_DEVICE_DIR "${STM32F1_CMSIS_DIR}/Device/ST/STM32F1xx")

# ============================================================================
# STM32F103 startup / system files
# ============================================================================

set(STM32F103_STARTUP_FILE "${STM32F1_DEVICE_DIR}/Source/Templates/gcc/startup_stm32f103xb.s")

set(STM32F1_SYSTEM_FILE "${STM32F1_DEVICE_DIR}/Source/Templates/system_stm32f1xx.c")

# ============================================================================
# STM32F103 linker script
# ============================================================================

set(STM32F103_LINKER_SCRIPT
    "${CMAKE_SOURCE_DIR}/platform/ports/stm32/stm32f103/startup/STM32F103C8Tx_FLASH.ld"
)

# ============================================================================
# Validate STM32CubeF1 dependency
# ============================================================================

if(NOT EXISTS "${STM32CUBE_F1_DIR}")

    message(FATAL_ERROR "STM32CubeF1 dependency not found: ${STM32CUBE_F1_DIR}")

endif()

if(NOT EXISTS "${STM32F103_STARTUP_FILE}")

    message(FATAL_ERROR "STM32F103 GCC startup file not found: ${STM32F103_STARTUP_FILE}")

endif()

if(NOT EXISTS "${STM32F1_SYSTEM_FILE}")

    message(FATAL_ERROR "STM32F1 system file not found: ${STM32F1_SYSTEM_FILE}")

endif()

if(NOT EXISTS "${STM32F103_LINKER_SCRIPT}")

    message(FATAL_ERROR "STM32F103 linker script not found: ${STM32F103_LINKER_SCRIPT}")

endif()

# ============================================================================
# MCU compiler flags
# ============================================================================

set(STM32F103_CPU_FLAGS -mcpu=cortex-m3 -mthumb)

set(STM32F103_SECTION_FLAGS -ffunction-sections -fdata-sections)

set(STM32F103_CXX_FLAGS -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics)

# ============================================================================
# Configure an STM32F103 target
#
# Applies MCU-specific compile options to the specified target.
# ============================================================================

function(stm32f103_configure_target TARGET)

    if(NOT TARGET ${TARGET})

        message(FATAL_ERROR "stm32f103_configure_target(): target '${TARGET}' does not exist")

    endif()

    target_compile_options(
        ${TARGET}
        PRIVATE ${STM32F103_CPU_FLAGS}
                ${STM32F103_SECTION_FLAGS}
                $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
                $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
                $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
                $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
    )

endfunction()

# ============================================================================
# Configure an STM32F103 executable
#
# Applies MCU compile options and firmware linker options.
# ============================================================================

function(stm32f103_configure_executable TARGET)

    if(NOT TARGET ${TARGET})

        message(FATAL_ERROR "stm32f103_configure_executable(): target '${TARGET}' does not exist")

    endif()

    stm32f103_configure_target(${TARGET})

    set(MAP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.map")

    target_link_options(
        ${TARGET}
        PRIVATE
        ${STM32F103_CPU_FLAGS}
        -T${STM32F103_LINKER_SCRIPT}
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        -Wl,-Map=${MAP_FILE}
        --specs=nano.specs
    )

endfunction()

# ============================================================================
# Generate firmware artifacts
#
# Produces:
#   <target>.hex
#   <target>.bin
#   linker memory usage output
# ============================================================================

function(stm32f103_add_firmware_artifacts TARGET)

    if(NOT TARGET ${TARGET})

        message(FATAL_ERROR "stm32f103_add_firmware_artifacts(): target '${TARGET}' does not exist")

    endif()

    add_custom_command(
        TARGET ${TARGET}
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET}>
                ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET}>
                ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET}>
        COMMENT "Generating STM32F103 firmware images"
    )

endfunction()
