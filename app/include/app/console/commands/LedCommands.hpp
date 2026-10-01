#pragma once

#include "platform/common/Result.hpp"
#include "platform/middleware/cli/CommandRegistry.hpp"
#include "platform/middleware/cli/IConsole.hpp"

namespace app::console::commands
{

class LedCommands
{
public:
    explicit LedCommands(platform::middleware::cli::IConsole& console) noexcept;

    platform::Result<void>
    registerCommands(platform::middleware::cli::CommandRegistry& registry) noexcept;

    [[nodiscard]] bool blinkingEnabled() const noexcept;

private:
    static void enableBlinking(void* context);
    static void disableBlinking(void* context);

    platform::middleware::cli::IConsole& console_;
    bool blinkingEnabled_{true};
};

} // namespace app::console::commands