#pragma once

#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::os
{

using EventMask = std::uint32_t;

class IEvent
{
public:
    virtual ~IEvent() = default;

    virtual platform::Result<void> set(EventMask events) = 0;

    virtual platform::Result<EventMask> wait(EventMask events, std::uint32_t timeoutMs) = 0;

    virtual platform::Result<void> clear(EventMask events) = 0;
};

} // namespace platform::os