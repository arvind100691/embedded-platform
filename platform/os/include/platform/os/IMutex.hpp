#pragma once

#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::os
{

class IMutex
{
public:
    virtual ~IMutex() = default;

    virtual platform::Result<void> lock(
        std::uint32_t timeoutMs) = 0;

    virtual platform::Result<void> unlock() = 0;
};

} // namespace platform::os