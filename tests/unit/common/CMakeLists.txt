add_executable(platform_common_unit_tests TypesTest.cpp ErrorCodeTest.cpp ResultTest.cpp)

target_link_libraries(
    platform_common_unit_tests PRIVATE platform_common GTest::gtest_main GTest::gmock
)

target_compile_features(platform_common_unit_tests PRIVATE cxx_std_17)

gtest_discover_tests(platform_common_unit_tests)
