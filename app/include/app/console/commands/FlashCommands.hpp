#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "platform/common/Result.hpp"
#include "platform/hal/IFlashStorage.hpp"
#include "platform/middleware/cli/CommandRegistry.hpp"
#include "platform/middleware/cli/IConsole.hpp"

namespace app::console::commands
{

class FlashCommands
{
public:
    static constexpr std::size_t kMaxReadBytes = 16U;
    static constexpr std::size_t kMaxWriteBytes = 16U;

    FlashCommands(platform::middleware::cli::IConsole& console,
                  platform::hal::IFlashStorage& flash) noexcept;

    platform::Result<void>
    registerCommands(platform::middleware::cli::CommandRegistry& registry) noexcept;

private:
    static void handle(void* context, std::string_view arguments);

    void execute(std::string_view arguments);
    void writeUsage();
    void writeError(platform::ErrorCode error);
    void writeReadData(std::uint32_t address, const std::uint8_t* data, std::size_t size);

    platform::middleware::cli::IConsole& console_;
    platform::hal::IFlashStorage& flash_;
};

} // namespace app::console::commands