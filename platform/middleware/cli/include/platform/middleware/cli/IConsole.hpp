#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "platform/common/Result.hpp"

namespace platform::middleware::cli
{

class IConsole
{
public:
    virtual ~IConsole() = default;

    virtual platform::Result<void> write(std::string_view text) = 0;

    virtual platform::Result<std::size_t> read(std::uint8_t* data, std::size_t size,
                                               std::uint32_t timeoutMs) = 0;
};

} // namespace platform::middleware::cli