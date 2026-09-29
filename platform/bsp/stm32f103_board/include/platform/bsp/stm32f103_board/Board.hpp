#pragma once

#include "platform/hal/IGpio.hpp"
#include "platform/hal/IUart.hpp"

namespace platform::bsp::stm32f103_board
{

void init();

platform::hal::IGpio& statusLed();

platform::hal::IUart& consoleUart();

} // namespace platform::bsp::stm32f103_board
