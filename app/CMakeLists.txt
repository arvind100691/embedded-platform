# ============================================================================
# Host application
# ============================================================================

if(NOT BUILD_STM32F103)

    add_executable(embedded_platform src/main.cpp)

    target_include_directories(embedded_platform PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(embedded_platform PRIVATE platform_common)

endif()

# ============================================================================
# STM32F103 firmware
# ============================================================================

if(BUILD_STM32F103)

    # ------------------------------------------------------------------------
    # Firmware executable
    # ------------------------------------------------------------------------

    add_executable(
        embedded_firmware src/Application.cpp src/FaultHandler.cpp
                          src/console/commands/LedCommands.cpp src/target_main.cpp
                          ${STM32F103_STARTUP_FILE}
                          ${CMAKE_SOURCE_DIR}/platform/ports/stm32/stm32f103/src/syscalls.c
    )

    set_target_properties(
        embedded_firmware PROPERTIES OUTPUT_NAME "embedded_firmware" SUFFIX ".elf"
    )

    # ------------------------------------------------------------------------
    # Includes
    # ------------------------------------------------------------------------

    target_include_directories(embedded_firmware PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    # ------------------------------------------------------------------------
    # Dependencies
    # ------------------------------------------------------------------------

    target_link_libraries(
        embedded_firmware PRIVATE platform_common platform_hal platform_os platform_stm32f103
                                  platform_cli bsp_stm32f103_board
    )

    target_compile_features(embedded_firmware PRIVATE cxx_std_17)

    # ------------------------------------------------------------------------
    # STM32F103 compiler + linker configuration
    # ------------------------------------------------------------------------

    stm32f103_configure_executable(embedded_firmware)

    # ------------------------------------------------------------------------
    # HEX / BIN generation
    # ------------------------------------------------------------------------

    stm32f103_add_firmware_artifacts(embedded_firmware)

endif()
