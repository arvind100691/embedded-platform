#pragma once

#include <cstdint>

namespace platform::common
{

struct FaultRegisters
{
    std::uint32_t r0{0U};
    std::uint32_t r1{0U};
    std::uint32_t r2{0U};
    std::uint32_t r3{0U};
    std::uint32_t r12{0U};
    std::uint32_t lr{0U};
    std::uint32_t pc{0U};
    std::uint32_t psr{0U};
    std::uint32_t cfsr{0U};
    std::uint32_t hfsr{0U};
    std::uint32_t dfsr{0U};
    std::uint32_t afsr{0U};
    std::uint32_t mmfar{0U};
    std::uint32_t bfar{0U};
    std::uint32_t shcsr{0U};
};

} // namespace platform::common
