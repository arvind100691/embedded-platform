add_library(platform_communication INTERFACE)

target_include_directories(
    platform_communication
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    platform_communication
    INTERFACE
        platform_hal
)

target_compile_features(
    platform_communication
    INTERFACE
        cxx_std_17
)