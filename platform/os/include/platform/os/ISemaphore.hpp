#pragma once

#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::os
{

class ISemaphore
{
public:
    virtual ~ISemaphore() = default;

    virtual platform::Result<void> acquire(std::uint32_t timeoutMs) = 0;

    virtual platform::Result<void> release() = 0;
};

} // namespace platform::os