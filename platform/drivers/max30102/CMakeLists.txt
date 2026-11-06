add_library(
    platform_max30102
    src/Max30102.cpp
)

target_include_directories(
    platform_max30102
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(platform_max30102 PUBLIC platform_hal)

target_compile_features(platform_max30102 PUBLIC cxx_std_17)

if(BUILD_STM32F103)
    stm32f103_configure_target(platform_max30102)
endif()
