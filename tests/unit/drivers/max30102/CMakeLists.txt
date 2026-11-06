add_executable(platform_max30102_unit_tests Max30102Test.cpp)

target_include_directories(
    platform_max30102_unit_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_link_libraries(
    platform_max30102_unit_tests
    PRIVATE
        platform_max30102
        GTest::gtest_main
)

target_compile_features(platform_max30102_unit_tests PRIVATE cxx_std_17)

gtest_discover_tests(platform_max30102_unit_tests)
