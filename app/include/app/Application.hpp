#pragma once

#include <string_view>

#include "platform/hal/IGpio.hpp"
#include "platform/middleware/cli/IConsole.hpp"

#include "app/console/commands/LedCommands.hpp"

namespace app
{

class Application
{
public:
    Application(platform::hal::IGpio& statusLed, platform::middleware::cli::IConsole& console);

    bool initialize();

    [[nodiscard]] bool blinkingEnabled() const noexcept;

    int run();

private:
    platform::hal::IGpio& statusLed_;
    platform::middleware::cli::IConsole& console_;
    app::console::commands::LedCommands ledCommands_;
};

} // namespace app