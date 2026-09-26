add_library(platform_os INTERFACE)

target_include_directories(platform_os INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(platform_os INTERFACE platform_common)

target_compile_features(platform_os INTERFACE cxx_std_17)
