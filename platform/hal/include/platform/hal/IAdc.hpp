#pragma once

#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::hal
{

enum class AdcResolution
{
    Bits8 = 8,
    Bits10 = 10,
    Bits12 = 12,
    Bits16 = 16
};

struct AdcConfig
{
    AdcResolution resolution{
        AdcResolution::Bits12
    };
};

class IAdc
{
public:
    virtual ~IAdc() = default;

    virtual platform::Result<void> configure(
        const AdcConfig& config) = 0;

    virtual platform::Result<std::uint32_t> read(
        std::uint32_t channel) = 0;
};

} // namespace platform::hal