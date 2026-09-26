#pragma once

#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::hal
{

struct PwmConfig
{
    std::uint32_t frequencyHz{1'000};

    std::uint16_t dutyCyclePermille{0};
};

class IPwm
{
public:
    virtual ~IPwm() = default;

    virtual platform::Result<void> configure(const PwmConfig& config) = 0;

    virtual platform::Result<void> setDutyCycle(std::uint16_t dutyCyclePermille) = 0;

    virtual platform::Result<void> enable() = 0;

    virtual platform::Result<void> disable() = 0;
};

} // namespace platform::hal