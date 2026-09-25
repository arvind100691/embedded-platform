# ============================================================================
# Host application
# ============================================================================

if(NOT BUILD_STM32F103)

    add_executable(
        embedded_platform
        src/main.cpp
    )

    target_include_directories(
        embedded_platform
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(
        embedded_platform
        PRIVATE
            platform_common
    )

endif()

# ============================================================================
# STM32F103 firmware
# ============================================================================

if(BUILD_STM32F103)

    # ------------------------------------------------------------------------
    # STM32F103 startup file
    # ------------------------------------------------------------------------

    set(
        STM32F103_STARTUP_FILE
        ${CMAKE_SOURCE_DIR}/external/stm32cube-f1/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s
    )

    if(NOT EXISTS "${STM32F103_STARTUP_FILE}")

        message(
            FATAL_ERROR
            "STM32F103 startup file not found: "
            "${STM32F103_STARTUP_FILE}"
        )

    endif()

    # ------------------------------------------------------------------------
    # Firmware executable
    # ------------------------------------------------------------------------

    add_executable(
        embedded_firmware

        src/Application.cpp
        src/target_main.cpp

        ${STM32F103_STARTUP_FILE}
    )

    set_target_properties(
        embedded_firmware
        PROPERTIES
            OUTPUT_NAME "embedded_firmware"
            SUFFIX ".elf"
    )

    target_include_directories(
        embedded_firmware
        PRIVATE

            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    # ------------------------------------------------------------------------
    # Dependencies
    # ------------------------------------------------------------------------

    target_link_libraries(
        embedded_firmware
        PRIVATE

            platform_common
            platform_hal
            platform_os

            platform_stm32f103
            bsp_stm32f103_board
    )

    # ------------------------------------------------------------------------
    # ARM compiler options
    # ------------------------------------------------------------------------

    target_compile_options(
        embedded_firmware
        PRIVATE

            -mcpu=cortex-m3
            -mthumb

            -ffunction-sections
            -fdata-sections

            -fno-exceptions
            -fno-rtti
            -fno-use-cxa-atexit
            -fno-threadsafe-statics
    )

    # ------------------------------------------------------------------------
    # ARM linker options
    # ------------------------------------------------------------------------

    set(
        STM32F103_LINKER_SCRIPT
        ${CMAKE_SOURCE_DIR}/platform/ports/stm32/stm32f103/startup/STM32F103C8Tx_FLASH.ld
    )

    if(NOT EXISTS "${STM32F103_LINKER_SCRIPT}")

        message(
            FATAL_ERROR
            "STM32F103 linker script not found: "
            "${STM32F103_LINKER_SCRIPT}"
        )

    endif()

    target_link_options(
        embedded_firmware
        PRIVATE

            -mcpu=cortex-m3
            -mthumb

            -T${STM32F103_LINKER_SCRIPT}

            -Wl,--gc-sections
            -Wl,--print-memory-usage

            -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/embedded_firmware.map

            --specs=nano.specs
            --specs=nosys.specs
    )

    # ------------------------------------------------------------------------
    # Generate HEX/BIN
    # ------------------------------------------------------------------------

    add_custom_command(
        TARGET embedded_firmware
        POST_BUILD

        COMMAND
            ${CMAKE_OBJCOPY}
            -O ihex
            $<TARGET_FILE:embedded_firmware>
            ${CMAKE_CURRENT_BINARY_DIR}/embedded_firmware.hex

        COMMAND
            ${CMAKE_OBJCOPY}
            -O binary
            $<TARGET_FILE:embedded_firmware>
            ${CMAKE_CURRENT_BINARY_DIR}/embedded_firmware.bin

        COMMAND
            ${CMAKE_SIZE}
            $<TARGET_FILE:embedded_firmware>

        COMMENT
            "Generating STM32F103 firmware images"
    )

endif()