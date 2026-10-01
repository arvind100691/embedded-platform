#include "app/console/commands/LedCommands.hpp"

namespace app::console::commands
{

LedCommands::LedCommands(platform::middleware::cli::IConsole& console) noexcept
    : console_(console)
{
}

platform::Result<void>
LedCommands::registerCommands(platform::middleware::cli::CommandRegistry& registry) noexcept
{
    const auto enableResult =
        registry.registerCommand("blink on", &LedCommands::enableBlinking, this);
    if (!enableResult)
    {
        return enableResult;
    }

    return registry.registerCommand("blink off", &LedCommands::disableBlinking, this);
}

bool LedCommands::blinkingEnabled() const noexcept
{
    return blinkingEnabled_;
}

void LedCommands::enableBlinking(void* context)
{
    auto* commands = static_cast<LedCommands*>(context);
    commands->blinkingEnabled_ = true;
    (void)commands->console_.write("LED blinking enabled\r\n");
}

void LedCommands::disableBlinking(void* context)
{
    auto* commands = static_cast<LedCommands*>(context);
    commands->blinkingEnabled_ = false;
    (void)commands->console_.write("LED blinking disabled\r\n");
}

} // namespace app::console::commands