#pragma once

#include "platform/common/Result.hpp"

namespace platform::hal
{

enum class GpioDirection
{
    Input,
    Output
};

enum class GpioState
{
    Low,
    High
};

enum class GpioPull
{
    None,
    PullUp,
    PullDown
};

enum class GpioInterruptEdge
{
    None,
    Rising,
    Falling,
    Both
};

using GpioInterruptCallback = void (*)(void* context);

struct GpioConfig
{
    GpioDirection direction{GpioDirection::Input};

    GpioPull pull{GpioPull::None};

    GpioState initialState{GpioState::Low};

    GpioInterruptEdge interruptEdge{GpioInterruptEdge::None};
};

class IGpio
{
public:
    virtual ~IGpio() = default;

    virtual platform::Result<void> configure(const GpioConfig& config) = 0;

    virtual platform::Result<void> write(GpioState state) = 0;

    virtual platform::Result<GpioState> read() const = 0;

    virtual platform::Result<void> toggle() = 0;

    virtual platform::Result<void> registerInterruptCallback(GpioInterruptCallback callback,
                                                             void* context) = 0;

    virtual platform::Result<void> enableInterrupt() = 0;

    virtual platform::Result<void> disableInterrupt() = 0;
};

} // namespace platform::hal