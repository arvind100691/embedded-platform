#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "platform/hal/IUart.hpp"
#include "platform/middleware/cli/IConsole.hpp"

namespace platform::middleware::cli
{

class UartConsole final : public IConsole
{
public:
    explicit UartConsole(platform::hal::IUart& uart,
                         platform::hal::UartConfig config = platform::hal::UartConfig{},
                         std::uint32_t writeTimeoutMs = 100U) noexcept
        : uart_(uart)
        , config_(config)
        , writeTimeoutMs_(writeTimeoutMs)
    {
    }

    platform::Result<void> initialize()
    {
        initialized_ = false;
        const auto result = uart_.configure(config_);
        if (result)
        {
            initialized_ = true;
        }
        return result;
    }

    platform::Result<void> write(std::string_view text) override
    {
        if (!initialized_)
        {
            return platform::Result<void>::failure(platform::ErrorCode::NotReady);
        }

        if (text.empty())
        {
            return platform::Result<void>::success();
        }

        return uart_.transmit(reinterpret_cast<const std::uint8_t*>(text.data()), text.size(),
                              writeTimeoutMs_);
    }

    platform::Result<std::size_t> read(std::uint8_t* data, std::size_t size,
                                       std::uint32_t timeoutMs) override
    {
        if (!initialized_)
        {
            return platform::Result<std::size_t>::failure(platform::ErrorCode::NotReady);
        }

        if (data == nullptr || size == 0U)
        {
            return platform::Result<std::size_t>::failure(platform::ErrorCode::InvalidArgument);
        }

        return uart_.receive(data, size, timeoutMs);
    }

private:
    platform::hal::IUart& uart_;
    platform::hal::UartConfig config_;
    std::uint32_t writeTimeoutMs_;
    bool initialized_{false};
};

} // namespace platform::middleware::cli