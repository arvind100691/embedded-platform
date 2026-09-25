add_library(platform_hal INTERFACE)

target_include_directories(platform_hal
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(platform_hal
    INTERFACE
        platform_common
)

target_compile_features(platform_hal
    INTERFACE
        cxx_std_17
)