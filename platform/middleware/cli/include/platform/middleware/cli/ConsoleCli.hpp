#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "platform/common/Result.hpp"
#include "platform/middleware/cli/CommandRegistry.hpp"
#include "platform/middleware/cli/IConsole.hpp"

namespace platform::middleware::cli
{

class ConsoleCli
{
public:
    ConsoleCli(IConsole& console, const CommandRegistry& commands) noexcept
        : console_(console)
        , commands_(commands)
    {
    }

    platform::Result<void> start()
    {
        return console_.write("> ");
    }

    platform::Result<void> poll()
    {
        std::uint8_t input = 0U;
        const auto result = console_.read(&input, 1U, 0U);
        if (!result)
        {
            if (result.error() == platform::ErrorCode::Timeout ||
                result.error() == platform::ErrorCode::BufferEmpty)
            {
                return platform::Result<void>::success();
            }
            return platform::Result<void>::failure(result.error());
        }

        if (result.value() == nullptr || *result.value() == 0U)
        {
            return platform::Result<void>::success();
        }

        return processInput(input);
    }

private:
    platform::Result<void> processInput(std::uint8_t input)
    {
        if (input == static_cast<std::uint8_t>('\n') && previousWasCarriageReturn_)
        {
            previousWasCarriageReturn_ = false;
            return platform::Result<void>::success();
        }

        previousWasCarriageReturn_ = input == static_cast<std::uint8_t>('\r');
        if (input == static_cast<std::uint8_t>('\r') || input == static_cast<std::uint8_t>('\n'))
        {
            const auto newlineResult = console_.write("\r\n");
            if (!newlineResult)
            {
                return newlineResult;
            }

            if (overflowed_)
            {
                const auto errorResult = console_.write("error: command too long\r\n");
                if (!errorResult)
                {
                    return errorResult;
                }
            }
            else if (lineSize_ > 0U)
            {
                const auto commandResult =
                    commands_.dispatch(std::string_view(line_.data(), lineSize_));
                if (!commandResult && commandResult.error() == platform::ErrorCode::NotFound)
                {
                    const auto errorResult = console_.write("Unknown command\r\n");
                    if (!errorResult)
                    {
                        return errorResult;
                    }
                }
                else if (!commandResult)
                {
                    return commandResult;
                }
            }

            lineSize_ = 0U;
            overflowed_ = false;
            return console_.write("> ");
        }

        if (input == 0x08U || input == 0x7FU)
        {
            if (lineSize_ > 0U)
            {
                --lineSize_;
                return console_.write("\b \b");
            }
            return platform::Result<void>::success();
        }

        if (input < 0x20U || input > 0x7EU)
        {
            return platform::Result<void>::success();
        }

        if (lineSize_ == line_.size())
        {
            overflowed_ = true;
            return platform::Result<void>::success();
        }

        line_[lineSize_++] = static_cast<char>(input);
        const char character = static_cast<char>(input);
        return console_.write(std::string_view(&character, 1U));
    }

    IConsole& console_;
    const CommandRegistry& commands_;
    std::array<char, 64U> line_{};
    std::size_t lineSize_{0U};
    bool overflowed_{false};
    bool previousWasCarriageReturn_{false};
};

} // namespace platform::middleware::cli