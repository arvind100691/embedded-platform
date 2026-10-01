#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "platform/common/Result.hpp"

namespace platform::middleware::cli
{

class CommandRegistry
{
public:
    using Handler = void (*)(void* context);

    static constexpr std::size_t kMaxCommands = 8U;
    static constexpr std::size_t kMaxCommandLength = 32U;

    platform::Result<void> registerCommand(std::string_view name, Handler handler,
                                           void* context) noexcept
    {
        if (name.empty() || name.size() > kMaxCommandLength || handler == nullptr)
        {
            return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
        }

        for (std::size_t index = 0U; index < commandCount_; ++index)
        {
            if (commandName(index) == name)
            {
                return platform::Result<void>::failure(platform::ErrorCode::InvalidState);
            }
        }

        if (commandCount_ == commands_.size())
        {
            return platform::Result<void>::failure(platform::ErrorCode::BufferFull);
        }

        Command& command = commands_[commandCount_];
        for (std::size_t index = 0U; index < name.size(); ++index)
        {
            command.name[index] = name[index];
        }
        command.nameSize = name.size();
        command.handler = handler;
        command.context = context;
        ++commandCount_;

        return platform::Result<void>::success();
    }

    platform::Result<void> dispatch(std::string_view name) const noexcept
    {
        for (std::size_t index = 0U; index < commandCount_; ++index)
        {
            if (commandName(index) == name)
            {
                commands_[index].handler(commands_[index].context);
                return platform::Result<void>::success();
            }
        }

        return platform::Result<void>::failure(platform::ErrorCode::NotFound);
    }

private:
    struct Command
    {
        std::array<char, kMaxCommandLength> name{};
        std::size_t nameSize{0U};
        Handler handler{nullptr};
        void* context{nullptr};
    };

    [[nodiscard]] std::string_view commandName(std::size_t index) const noexcept
    {
        return std::string_view(commands_[index].name.data(), commands_[index].nameSize);
    }

    std::array<Command, kMaxCommands> commands_{};
    std::size_t commandCount_{0U};
};

} // namespace platform::middleware::cli