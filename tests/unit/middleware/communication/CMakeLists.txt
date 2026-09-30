add_executable(
    platform_buffered_async_uart_unit_tests
    BufferedAsyncUartTest.cpp
)

target_include_directories(
    platform_buffered_async_uart_unit_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_link_libraries(
    platform_buffered_async_uart_unit_tests
    PRIVATE
        platform_communication
        GTest::gtest_main
)

target_compile_features(
    platform_buffered_async_uart_unit_tests
    PRIVATE
        cxx_std_17
)

gtest_discover_tests(platform_buffered_async_uart_unit_tests)