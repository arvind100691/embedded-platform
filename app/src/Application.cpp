#include "app/Application.hpp"

namespace app
{

Application::Application(
    platform::hal::IGpio& statusLed)
    : statusLed_(statusLed)
{
}

bool Application::initialize()
{
    platform::hal::GpioConfig config{};

    config.direction =
        platform::hal::GpioDirection::Output;

    config.pull =
        platform::hal::GpioPull::None;

    config.initialState =
        platform::hal::GpioState::High;

    config.interruptEdge =
        platform::hal::GpioInterruptEdge::None;

    const auto result =
        statusLed_.configure(config);

    return result.hasValue();
}

} // namespace app