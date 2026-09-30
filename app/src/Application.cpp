#include "app/Application.hpp"

#include <cstdint>

namespace
{

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

Application::Application(platform::hal::IGpio& statusLed)
    : statusLed_(statusLed)
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

int Application::run()
{
    if (!initialize())
    {
        while (true)
        {
        }
    }

    while (true)
    {
        statusLed_.write(platform::hal::GpioState::Low);
        delayLoop(500000U);

        statusLed_.write(platform::hal::GpioState::High);
        delayLoop(500000U);
    }

    return 0;
}

} // namespace app