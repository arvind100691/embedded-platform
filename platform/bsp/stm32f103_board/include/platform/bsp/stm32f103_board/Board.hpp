#pragma once

#include "platform/hal/IGpio.hpp"

namespace platform::bsp::stm32f103_board
{

void init();

platform::hal::IGpio& statusLed();

} // namespace platform::bsp::stm32f103_board