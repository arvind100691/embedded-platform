add_executable(
    application_unit_tests ApplicationTest.cpp LedCommandsTest.cpp
    ${CMAKE_SOURCE_DIR}/app/src/Application.cpp
    ${CMAKE_SOURCE_DIR}/app/src/console/commands/LedCommands.cpp
)

target_include_directories(
    application_unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/app/include ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_link_libraries(application_unit_tests PRIVATE platform_hal platform_cli GTest::gtest_main)

target_compile_features(application_unit_tests PRIVATE cxx_std_17)

gtest_discover_tests(application_unit_tests)
