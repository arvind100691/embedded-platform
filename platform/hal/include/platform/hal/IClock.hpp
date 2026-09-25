#pragma once

#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::hal
{

class IClock
{
public:
    virtual ~IClock() = default;

    virtual platform::Result<std::uint64_t>
    nowMicros() const = 0;

    virtual platform::Result<std::uint64_t>
    nowMillis() const = 0;
};

} // namespace platform::hal