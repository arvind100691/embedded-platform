# ============================================================================
# STM32F103 board support package
# ============================================================================

add_library(bsp_stm32f103_board STATIC src/Board.cpp src/stm32f1xx_it.cpp)

target_include_directories(bsp_stm32f103_board PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(bsp_stm32f103_board PUBLIC platform_hal platform_stm32f103)

target_compile_features(bsp_stm32f103_board PUBLIC cxx_std_17)

stm32f103_configure_target(bsp_stm32f103_board)
