#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::os
{

class IQueue
{
public:
    virtual ~IQueue() = default;

    virtual platform::Result<void> send(
        const void* data,
        std::size_t size,
        std::uint32_t timeoutMs) = 0;

    virtual platform::Result<std::size_t> receive(
        void* data,
        std::size_t size,
        std::uint32_t timeoutMs) = 0;
};

} // namespace platform::os