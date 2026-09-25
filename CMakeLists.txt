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

option(BUILD_TESTING "Build unit and integration tests" ON)

add_subdirectory(platform/common)
add_subdirectory(platform/hal)
add_subdirectory(platform/os)

add_subdirectory(app)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()