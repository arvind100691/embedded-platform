#include <gtest/gtest.h>

#include "app/Application.hpp"
#include "platform/common/ErrorCode.hpp"
#include "platform/hal/IGpio.hpp"

#include "MockGpio.hpp"

namespace
{

TEST(
    ApplicationTest,
    InitializeConfiguresStatusLedAsOutput)
{
    MockGpio gpio;

    app::Application application(gpio);

    const bool result =
        application.initialize();

    EXPECT_TRUE(result);

    EXPECT_EQ(
        gpio.configureCallCount(),
        1U);

    const auto& config =
        gpio.lastConfig();

    EXPECT_EQ(
        config.direction,
        platform::hal::GpioDirection::Output);

    EXPECT_EQ(
        config.pull,
        platform::hal::GpioPull::None);

    EXPECT_EQ(
        config.initialState,
        platform::hal::GpioState::High);

    EXPECT_EQ(
        config.interruptEdge,
        platform::hal::GpioInterruptEdge::None);
}

TEST(
    ApplicationTest,
    InitializeFailsWhenGpioConfigurationFails)
{
    MockGpio gpio;

    gpio.setConfigureFailure(
        platform::ErrorCode::HardwareFault);

    app::Application application(gpio);

    const bool result =
        application.initialize();

    EXPECT_FALSE(result);

    EXPECT_EQ(
        gpio.configureCallCount(),
        1U);
}

TEST(
    ApplicationTest,
    InitializeSetsExpectedInitialState)
{
    MockGpio gpio;

    app::Application application(gpio);

    ASSERT_TRUE(
        application.initialize());

    EXPECT_EQ(
        gpio.state(),
        platform::hal::GpioState::High);
}

} // namespace