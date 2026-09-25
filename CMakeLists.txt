cmake_minimum_required(VERSION 3.22)

project(EmbeddedPlatform
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(embedded_platform
    app/src/main.cpp
)

target_include_directories(embedded_platform
    PRIVATE
        ${CMAKE_SOURCE_DIR}/app/include
)
