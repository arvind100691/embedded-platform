# ============================================================================
# STM32F103 board support package
# ============================================================================

add_library(
    bsp_stm32f103_board STATIC src/Board.cpp src/stm32f1xx_hal_msp.cpp
                               include/platform/bsp/stm32f103_board/Board.hpp
)

target_include_directories(bsp_stm32f103_board PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(bsp_stm32f103_board PUBLIC platform_hal platform_stm32f103)

target_compile_features(bsp_stm32f103_board PUBLIC cxx_std_17)

stm32f103_configure_target(bsp_stm32f103_board)

# ============================================================================
# Interrupt vector ownership
#
# These handlers must be retained as part of the board/MCU boundary even though
# they are not referenced by application code. Keeping them in a dedicated
# archive ensures the final firmware owns the vector layer in one place instead
# of requiring each integration test to include the IRQ source directly.
# ============================================================================

add_library(bsp_stm32f103_board_vectors STATIC src/stm32f1xx_it.cpp)

target_include_directories(bsp_stm32f103_board_vectors PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(bsp_stm32f103_board_vectors PUBLIC platform_hal platform_stm32f103)

target_compile_features(bsp_stm32f103_board_vectors PUBLIC cxx_std_17)

stm32f103_configure_target(bsp_stm32f103_board_vectors)
