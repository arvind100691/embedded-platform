#pragma once

#include "platform/common/Fault.hpp"

namespace platform::stm32f103
{

[[noreturn]] void handleHardFault();

platform::common::FaultRegisters captureFaultRegisters();

} // namespace platform::stm32f103
