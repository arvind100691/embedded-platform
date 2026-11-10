#include "app/Application.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "platform/common/ErrorCode.hpp"
#include "platform/hal/IGpio.hpp"
#include "platform/middleware/cli/UartConsole.hpp"

#include "MockFlashStorage.hpp"
#include "MockGpio.hpp"
#include "MockI2c.hpp"
#include "MockUart.hpp"

namespace
{

TEST(ApplicationTest, InitializeConfiguresStatusLedAsOutput)
{
    MockGpio gpio;
    MockFlashStorage flash;
    MockUart uart;
    platform::middleware::cli::UartConsole console(uart);
    ASSERT_TRUE(console.initialize());

    app::Application application(gpio, console, flash);

    const bool result = application.initialize();

    EXPECT_TRUE(result);

    EXPECT_EQ(gpio.configureCallCount(), 1U);

    const auto& config = gpio.lastConfig();

    EXPECT_EQ(config.direction, platform::hal::GpioDirection::Output);

    EXPECT_EQ(config.pull, platform::hal::GpioPull::None);

    EXPECT_EQ(config.initialState, platform::hal::GpioState::High);

    EXPECT_EQ(config.interruptEdge, platform::hal::GpioInterruptEdge::None);
}

TEST(ApplicationTest, InitializeFailsWhenGpioConfigurationFails)
{
    MockGpio gpio;
    MockFlashStorage flash;
    MockUart uart;
    platform::middleware::cli::UartConsole console(uart);
    ASSERT_TRUE(console.initialize());

    gpio.setConfigureFailure(platform::ErrorCode::HardwareFault);

    app::Application application(gpio, console, flash);

    const bool result = application.initialize();

    EXPECT_FALSE(result);

    EXPECT_EQ(gpio.configureCallCount(), 1U);
}

TEST(ApplicationTest, InitializeSetsExpectedInitialState)
{
    MockGpio gpio;
    MockFlashStorage flash;
    MockUart uart;
    platform::middleware::cli::UartConsole console(uart);
    ASSERT_TRUE(console.initialize());

    app::Application application(gpio, console, flash);

    ASSERT_TRUE(application.initialize());

    EXPECT_EQ(gpio.state(), platform::hal::GpioState::High);
}

TEST(ApplicationTest, LogsHeartRateAlongsideRawSensorSamples)
{
    constexpr std::uint8_t kPartIdRegister{0xFFU};
    constexpr std::uint8_t kFifoWritePointerRegister{0x04U};
    constexpr std::uint8_t kFifoOverflowCounterRegister{0x05U};
    constexpr std::uint8_t kFifoReadPointerRegister{0x06U};
    constexpr std::size_t kSamplesPerBatch{4U};
    constexpr std::size_t kSampleCount{1200U};

    MockGpio gpio;
    MockFlashStorage flash;
    MockUart uart;
    MockI2c i2c;
    platform::middleware::cli::UartConsole console(uart);
    ASSERT_TRUE(console.initialize());

    i2c.setRegister(kPartIdRegister, 0x15U);
    platform::drivers::max30102::Max30102 sensor(i2c);
    app::Application application(gpio, console, flash, &sensor);
    ASSERT_TRUE(application.initialize());

    for (std::size_t poll = 0U; poll < 13U; ++poll)
    {
        application.pollSensor();
    }
    ASSERT_TRUE(sensor.isReady());

    for (std::size_t batch = 0U; batch < kSampleCount / kSamplesPerBatch; ++batch)
    {
        std::vector<std::uint8_t> fifoData;
        fifoData.reserve(kSamplesPerBatch * 6U);
        for (std::size_t offset = 0U; offset < kSamplesPerBatch; ++offset)
        {
            const auto sampleIndex = batch * kSamplesPerBatch + offset;
            const auto phase = sampleIndex % 100U;
            const std::uint32_t infrared = 100000U + (phase == 10U ? 20000U : 0U);
            constexpr std::uint32_t kRed{90000U};
            for (const auto value : {kRed, infrared})
            {
                fifoData.push_back(static_cast<std::uint8_t>((value >> 16U) & 0x03U));
                fifoData.push_back(static_cast<std::uint8_t>(value >> 8U));
                fifoData.push_back(static_cast<std::uint8_t>(value));
            }
        }

        i2c.setRegister(kFifoWritePointerRegister,
                        static_cast<std::uint8_t>(kSamplesPerBatch));
        i2c.setRegister(kFifoOverflowCounterRegister, 0U);
        i2c.setRegister(kFifoReadPointerRegister, 0U);
        i2c.setFifoData(fifoData);
        application.pollSensor();
    }

    const auto& output = uart.transmittedData();
    const std::string log(output.begin(), output.end());
    EXPECT_NE(log.find("RED=90000 IR=100000 HR=--"), std::string::npos);
    EXPECT_NE(log.find("RED=90000 IR=100000 HR=60"), std::string::npos);
}

} // namespace