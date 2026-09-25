# Do not build GoogleTest's own test suite.
set(gtest_build_tests OFF CACHE BOOL "" FORCE)

# We only need GoogleTest/GMock libraries.
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)

add_subdirectory(
    ${CMAKE_SOURCE_DIR}/external/googletest
    ${CMAKE_BINARY_DIR}/external/googletest
    EXCLUDE_FROM_ALL
)

include(GoogleTest)

add_subdirectory(unit)