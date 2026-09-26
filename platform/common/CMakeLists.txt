add_library(platform_common INTERFACE)

target_include_directories(platform_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(platform_common INTERFACE cxx_std_17)
