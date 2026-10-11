#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "platform/common/ArgumentParser.hpp"
#include "platform/common/Result.hpp"

namespace platform::middleware::cli
{

class CommandRegistry
{
public:
    using Handler = void (*)(void* context);
    using ArgumentHandler = void (*)(void* context, std::string_view arguments);

    static constexpr std::size_t kMaxCommands = 8U;
    static constexpr std::size_t kMaxCommandLength = 32U;

    platform::Result<void> registerCommand(std::string_view name, Handler handler,
                                           void* context) noexcept
    {
        return registerCommand(name, handler, nullptr, context);
    }

    platform::Result<void> registerCommandWithArgs(std::string_view name, ArgumentHandler handler,
                                                   void* context) noexcept
    {
        platform::ArgumentParser parser(name);
        std::string_view commandToken;
        if (handler == nullptr || !parser.next(commandToken) || parser.hasRemaining())
        {
            return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
        }
        return registerCommand(name, nullptr, handler, context);
    }

    platform::Result<void> dispatch(std::string_view name) const noexcept
    {
        for (std::size_t index = 0U; index < commandCount_; ++index)
        {
            if (commandName(index) == name)
            {
                const Command& command = commands_[index];
                if (command.handler != nullptr)
                {
                    command.handler(command.context);
                }
                else
                {
                    command.argumentHandler(command.context, {});
                }
                return platform::Result<void>::success();
            }
        }

        for (std::size_t index = 0U; index < commandCount_; ++index)
        {
            const Command& command = commands_[index];
            if (command.argumentHandler == nullptr)
            {
                continue;
            }

            platform::ArgumentParser parser(name);
            std::string_view commandToken;
            if (parser.next(commandToken) && commandToken == commandName(index))
            {
                command.argumentHandler(command.context, parser.remaining());
                return platform::Result<void>::success();
            }
        }

        return platform::Result<void>::failure(platform::ErrorCode::NotFound);
    }

private:
    platform::Result<void> registerCommand(std::string_view name, Handler handler,
                                           ArgumentHandler argumentHandler, void* context) noexcept
    {
        if (name.empty() || name.size() > kMaxCommandLength ||
            (handler == nullptr && argumentHandler == nullptr))
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
        command.argumentHandler = argumentHandler;
        command.context = context;
        ++commandCount_;

        return platform::Result<void>::success();
    }
    struct Command
    {
        std::array<char, kMaxCommandLength> name{};
        std::size_t nameSize{0U};
        Handler handler{nullptr};
        ArgumentHandler argumentHandler{nullptr};
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