#pragma once

#include <cstdint>
#include <string_view>

#include "platform/drivers/max30102/Max30102.hpp"
#include "platform/hal/IFlashStorage.hpp"
#include "platform/hal/IGpio.hpp"
#include "platform/middleware/cli/IConsole.hpp"

#include "app/console/commands/FlashCommands.hpp"
#include "app/console/commands/LedCommands.hpp"

namespace app
{

class Application
{
public:
    Application(platform::hal::IGpio& statusLed, platform::middleware::cli::IConsole& console,
                platform::hal::IFlashStorage& flash,
                platform::drivers::max30102::Max30102* max30102 = nullptr);

    bool initialize();

    [[nodiscard]] bool blinkingEnabled() const noexcept;

    int run();

private:
    static void enableSensorStreaming(void* context);
    static void disableSensorStreaming(void* context);
    void pollSensor();

    platform::hal::IGpio& statusLed_;
    platform::middleware::cli::IConsole& console_;
    app::console::commands::FlashCommands flashCommands_;
    app::console::commands::LedCommands ledCommands_;
    platform::drivers::max30102::Max30102* max30102_;
    bool sensorOperational_{false};
    bool sensorStreaming_{false};
    bool sensorErrorReported_{false};
    std::uint32_t sensorRetryPolls_{0U};
};

} // namespace app