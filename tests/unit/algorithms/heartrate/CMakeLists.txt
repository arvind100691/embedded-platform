add_executable(platform_heart_rate_unit_tests HeartRateEstimatorTest.cpp)

target_link_libraries(
    platform_heart_rate_unit_tests
    PRIVATE platform_heart_rate GTest::gtest_main
)

target_compile_features(platform_heart_rate_unit_tests PRIVATE cxx_std_17)
gtest_discover_tests(platform_heart_rate_unit_tests)
