add_library(platform_cli INTERFACE)

target_include_directories(platform_cli INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(platform_cli INTERFACE platform_hal)

target_compile_features(platform_cli INTERFACE cxx_std_17)