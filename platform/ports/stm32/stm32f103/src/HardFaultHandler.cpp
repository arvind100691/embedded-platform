#include "platform/stm32f103/HardFaultHandler.hpp"

#include "stm32f1xx.h"

namespace platform::stm32f103
{

platform::common::FaultRegisters captureFaultRegisters()
{
    const std::uint32_t* stackFrame = reinterpret_cast<const std::uint32_t*>(__get_MSP());

    platform::common::FaultRegisters fault{};
    fault.r0 = stackFrame[0];
    fault.r1 = stackFrame[1];
    fault.r2 = stackFrame[2];
    fault.r3 = stackFrame[3];
    fault.r12 = stackFrame[4];
    fault.lr = stackFrame[5];
    fault.pc = stackFrame[6];
    fault.psr = stackFrame[7];
    fault.cfsr = SCB->CFSR;
    fault.hfsr = SCB->HFSR;
    fault.dfsr = SCB->DFSR;
    fault.afsr = SCB->AFSR;
    fault.mmfar = SCB->MMFAR;
    fault.bfar = SCB->BFAR;
    fault.shcsr = SCB->SHCSR;
    return fault;
}

} // namespace platform::stm32f103
