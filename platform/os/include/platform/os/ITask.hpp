#pragma once

#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::os
{

class ITask
{
public:
    virtual ~ITask() = default;

    virtual platform::Result<void> start() = 0;

    virtual platform::Result<void> stop() = 0;

    virtual platform::Result<void> delay(
        std::uint32_t milliseconds) = 0;
};

} // namespace platform::os