#include "platform/middleware/cli/UartConsole.hpp"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

#include "MockUart.hpp"

namespace
{

using platform::middleware::cli::UartConsole;

TEST(UartConsoleTest, InitializesUartBeforeConsoleUse)
{
    MockUart uart;
    UartConsole console(uart);

    EXPECT_EQ(console.write("boot").error(), platform::ErrorCode::NotReady);
    ASSERT_TRUE(console.initialize());

    EXPECT_TRUE(uart.configured());
    EXPECT_EQ(uart.lastConfig().baudRate, 115200U);
}

TEST(UartConsoleTest, WritesTextThroughUart)
{
    MockUart uart;
    UartConsole console(uart);
    ASSERT_TRUE(console.initialize());

    ASSERT_TRUE(console.write("ready\r\n"));

    const std::vector<std::uint8_t> expected{'r', 'e', 'a', 'd', 'y', '\r', '\n'};
    EXPECT_EQ(uart.transmittedData(), expected);
}

TEST(UartConsoleTest, ReadsBytesAndPropagatesUartErrors)
{
    MockUart uart;
    UartConsole console(uart);
    ASSERT_TRUE(console.initialize());

    const std::array<std::uint8_t, 2U> input{'o', 'k'};
    uart.enqueueReceivedData(input.data(), input.size());

    std::array<std::uint8_t, 2U> output{};
    const auto result = console.read(output.data(), output.size(), 25U);

    ASSERT_TRUE(result);
    EXPECT_EQ(*result.value(), output.size());
    EXPECT_EQ(output, input);

    uart.setReceiveFailure(platform::ErrorCode::CommunicationError);
    EXPECT_EQ(console.read(output.data(), output.size(), 25U).error(),
              platform::ErrorCode::CommunicationError);
}

} // namespace