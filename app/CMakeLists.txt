add_executable(embedded_platform
    src/main.cpp
)

target_include_directories(embedded_platform
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(embedded_platform
    PRIVATE
        platform_common
        platform_hal
        platform_os
)