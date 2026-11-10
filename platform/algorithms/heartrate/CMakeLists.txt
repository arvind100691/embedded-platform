add_library(
    platform_heart_rate
    STATIC
    src/HeartRateAlgorithm.cpp
    src/HeartRateEstimator.cpp
)

target_include_directories(
    platform_heart_rate
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(platform_heart_rate PUBLIC platform_common)
target_compile_features(platform_heart_rate PUBLIC cxx_std_17)

if(BUILD_STM32F103)
    stm32f103_configure_target(platform_heart_rate)
endif()
