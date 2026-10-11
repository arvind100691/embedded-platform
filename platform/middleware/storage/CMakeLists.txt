add_library(platform_storage STATIC src/W25Q128.cpp)

target_include_directories(platform_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(platform_storage PUBLIC platform_hal)

target_compile_features(platform_storage PUBLIC cxx_std_17)
