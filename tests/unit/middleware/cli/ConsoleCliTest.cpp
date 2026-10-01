#include "platform/middleware/cli/ConsoleCli.hpp"

#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <string_view>
#include <vector>

#include "platform/middleware/cli/UartConsole.hpp"

#include "MockUart.hpp"

namespace
{

void recordCommand(void* context)
{
    ++*static_cast<std::size_t*>(context);
}

TEST(ConsoleCliTest, EchoesAndDispatchesCarriageReturnLineFeedCommand)
{
    MockUart uart;
    platform::middleware::cli::UartConsole console(uart);
    ASSERT_TRUE(console.initialize());

    std::size_t commandCount = 0U;
    platform::middleware::cli::CommandRegistry commands;
    ASSERT_TRUE(commands.registerCommand("blink off", &recordCommand, &commandCount));
    platform::middleware::cli::ConsoleCli cli(console, commands);
    ASSERT_TRUE(cli.start());

    constexpr std::string_view input{"blink off\r\n"};
    uart.enqueueReceivedData(reinterpret_cast<const std::uint8_t*>(input.data()), input.size());
    for (std::size_t index = 0U; index < input.size(); ++index)
    {
        ASSERT_TRUE(cli.poll());
    }

    EXPECT_EQ(commandCount, 1U);

    const std::vector<std::uint8_t> expected{'>', ' ', 'b', 'l',  'i',  'n', 'k', ' ',
                                             'o', 'f', 'f', '\r', '\n', '>', ' '};
    EXPECT_EQ(uart.transmittedData(), expected);
}

} // namespace