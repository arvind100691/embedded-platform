#include "app/Application.hpp"

#include <cstdint>

#include "platform/middleware/cli/CommandRegistry.hpp"
#include "platform/middleware/cli/ConsoleCli.hpp"

namespace
{

constexpr std::uint32_t kDelayIterationsPerCliPoll = 25000U;
constexpr std::uint32_t kCliPollsPerBlinkInterval = 20U;

void delayLoop(std::uint32_t iterations)
{
    for (volatile std::uint32_t i = 0U; i < iterations; ++i)
    {
        __asm volatile("nop");
    }
}

} // namespace

namespace app
{

Application::Application(platform::hal::IGpio& statusLed,
                         platform::middleware::cli::IConsole& console)
    : statusLed_(statusLed)
    , console_(console)
    , ledCommands_(console)
{
}

bool Application::initialize()
{
    platform::hal::GpioConfig config{};

    config.direction = platform::hal::GpioDirection::Output;

    config.pull = platform::hal::GpioPull::None;

    config.initialState = platform::hal::GpioState::High;

    config.interruptEdge = platform::hal::GpioInterruptEdge::None;

    const auto result = statusLed_.configure(config);

    return result.hasValue();
}

bool Application::blinkingEnabled() const noexcept
{
    return ledCommands_.blinkingEnabled();
}

int Application::run()
{
    if (!initialize())
    {
        while (true)
        {
        }
    }

    platform::middleware::cli::CommandRegistry commands;
    if (!ledCommands_.registerCommands(commands))
    {
        while (true)
        {
        }
    }

    platform::middleware::cli::ConsoleCli cli(console_, commands);
    if (!cli.start())
    {
        while (true)
        {
        }
    }

    bool ledOn = false;
    while (true)
    {
        for (std::uint32_t interval = 0U; interval < kCliPollsPerBlinkInterval; ++interval)
        {
            (void)cli.poll();
            delayLoop(kDelayIterationsPerCliPoll);
        }

        if (ledCommands_.blinkingEnabled())
        {
            ledOn = !ledOn;
            statusLed_.write(ledOn ? platform::hal::GpioState::Low
                                   : platform::hal::GpioState::High);
        }
        else if (ledOn)
        {
            ledOn = false;
            statusLed_.write(platform::hal::GpioState::High);
        }
    }

    return 0;
}

} // namespace app