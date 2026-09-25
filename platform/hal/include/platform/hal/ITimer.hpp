#pragma once

#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::hal
{

enum class TimerMode
{
    OneShot,
    Periodic
};

using TimerCallback =
    void (*)(void* context);

struct TimerConfig
{
    std::uint32_t periodUs{1'000};

    TimerMode mode{
        TimerMode::Periodic
    };
};

class ITimer
{
public:
    virtual ~ITimer() = default;

    virtual platform::Result<void> configure(
        const TimerConfig& config) = 0;

    virtual platform::Result<void> registerCallback(
        TimerCallback callback,
        void* context) = 0;

    virtual platform::Result<void> start() = 0;

    virtual platform::Result<void> stop() = 0;
};

} // namespace platform::hal