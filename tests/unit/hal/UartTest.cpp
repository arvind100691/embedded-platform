#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

#include "MockUart.hpp"

namespace
{

platform::hal::UartConfig defaultConfig()
{
    platform::hal::UartConfig config{};
    config.baudRate = 115200U;
    config.dataBits = 8U;
    config.parity = platform::hal::UartParity::None;
    config.stopBits = platform::hal::UartStopBits::One;
    config.flowControl = platform::hal::UartFlowControl::None;
    return config;
}

} // namespace

TEST(MockUartTest, ConfigureStoresConfiguration)
{
    MockUart uart;
    const auto config = defaultConfig();

    const auto result = uart.configure(config);

    ASSERT_TRUE(result);
    EXPECT_TRUE(uart.configured());
    EXPECT_EQ(uart.configureCallCount(), 1U);
    EXPECT_EQ(uart.lastConfig().baudRate, 115200U);
    EXPECT_EQ(uart.lastConfig().dataBits, 8U);
    EXPECT_EQ(uart.lastConfig().parity, platform::hal::UartParity::None);
    EXPECT_EQ(uart.lastConfig().stopBits, platform::hal::UartStopBits::One);
    EXPECT_EQ(uart.lastConfig().flowControl, platform::hal::UartFlowControl::None);
}

TEST(MockUartTest, TransmitRecordsBytes)
{
    MockUart uart;
    const std::array<std::uint8_t, 4U> data{'T', 'E', 'S', 'T'};

    const auto result = uart.transmit(data.data(), data.size(), 100U);

    ASSERT_TRUE(result);
    EXPECT_EQ(uart.transmitCallCount(), 1U);
    EXPECT_EQ(uart.transmittedData(), std::vector<std::uint8_t>(data.begin(), data.end()));
}

TEST(MockUartTest, ReceiveReturnsQueuedBytes)
{
    MockUart uart;
    const std::array<std::uint8_t, 4U> queued{'U', 'A', 'R', 'T'};
    std::array<std::uint8_t, 4U> received{};

    uart.enqueueReceivedData(queued.data(), queued.size());

    const auto result = uart.receive(received.data(), received.size(), 100U);

    ASSERT_TRUE(result);
    ASSERT_NE(result.value(), nullptr);
    EXPECT_EQ(*result.value(), queued.size());
    EXPECT_EQ(received, queued);
}

TEST(MockUartTest, ReceiveCanReturnPartialData)
{
    MockUart uart;
    const std::array<std::uint8_t, 2U> queued{'O', 'K'};
    std::array<std::uint8_t, 4U> received{};

    uart.enqueueReceivedData(queued.data(), queued.size());

    const auto result = uart.receive(received.data(), received.size(), 100U);

    ASSERT_TRUE(result);
    ASSERT_NE(result.value(), nullptr);
    EXPECT_EQ(*result.value(), 2U);
    EXPECT_EQ(received[0], 'O');
    EXPECT_EQ(received[1], 'K');
}

TEST(MockUartTest, ReceiveWithoutDataReturnsBufferEmpty)
{
    MockUart uart;
    std::uint8_t received = 0U;

    const auto result = uart.receive(&received, 1U, 10U);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::BufferEmpty);
}

TEST(MockUartTest, ConfigurationFailureIsPropagated)
{
    MockUart uart;
    uart.setConfigureFailure(platform::ErrorCode::InvalidArgument);

    const auto result = uart.configure(defaultConfig());

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::InvalidArgument);
}

TEST(MockUartTest, TransmitFailureIsPropagated)
{
    MockUart uart;
    uart.setTransmitFailure(platform::ErrorCode::CommunicationError);

    const std::uint8_t byte = 'A';
    const auto result = uart.transmit(&byte, 1U, 10U);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::CommunicationError);
}

TEST(MockUartTest, ReceiveFailureIsPropagated)
{
    MockUart uart;
    uart.setReceiveFailure(platform::ErrorCode::Timeout);

    std::uint8_t byte = 0U;
    const auto result = uart.receive(&byte, 1U, 10U);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::Timeout);
}
