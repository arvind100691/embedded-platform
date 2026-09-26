cmake_minimum_required(VERSION 3.22)

project(EmbeddedPlatform
    VERSION 0.1.0
    DESCRIPTION "Portable layered embedded platform"
    LANGUAGES C CXX ASM
)

# ============================================================================
# Language standards
# ============================================================================

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# Project options
# ============================================================================

option(
    BUILD_TESTING
    "Build unit and integration tests"
    ON
)

option(
    BUILD_STM32F103
    "Build STM32F103 firmware"
    OFF
)

option(
    BUILD_STM32F103_INTEGRATION_TESTS
    "Build STM32F103 hardware integration tests"
    OFF
)

# ============================================================================
# Platform layers
# ============================================================================

add_subdirectory(platform/common)
add_subdirectory(platform/hal)
add_subdirectory(platform/os)

# ============================================================================
# STM32F103 platform + BSP
#
# Added exactly once.
# ============================================================================

if(
    BUILD_STM32F103
    OR BUILD_STM32F103_INTEGRATION_TESTS
)

    add_subdirectory(
        platform/ports/stm32/stm32f103
    )

    add_subdirectory(
        platform/bsp/stm32f103_board
    )

endif()

# ============================================================================
# Application
# ============================================================================

add_subdirectory(
    app
)

# ============================================================================
# Host tests
# ============================================================================

if(BUILD_TESTING)

    include(CTest)

    enable_testing()

    add_subdirectory(
        tests
    )

endif()

# ============================================================================
# Hardware integration tests
# ============================================================================

if(BUILD_STM32F103_INTEGRATION_TESTS)

    add_subdirectory(
        tests/integration/platform/stm32f103
    )

endif()