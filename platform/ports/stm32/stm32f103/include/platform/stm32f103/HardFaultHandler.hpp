#pragma once

#include <cstdint>

#include "platform/common/Fault.hpp"

namespace platform::stm32f103
{

[[noreturn]] void handleHardFault();

platform::common::FaultRegisters captureFaultRegisters(const std::uint32_t* stackFrame);

} // namespace platform::stm32f103
