#pragma once

#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::hal
{

struct WatchdogConfig
{
    std::uint32_t timeoutMs{1'000};
};

class IWatchdog
{
public:
    virtual ~IWatchdog() = default;

    virtual platform::Result<void> configure(
        const WatchdogConfig& config) = 0;

    virtual platform::Result<void> start() = 0;

    virtual platform::Result<void> refresh() = 0;
};

} // namespace platform::hal