#include "app/console/commands/FlashCommands.hpp"

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "platform/middleware/cli/UartConsole.hpp"

#include "MockFlashStorage.hpp"
#include "MockUart.hpp"

namespace
{

std::string transmittedText(const MockUart& uart)
{
    const auto& data = uart.transmittedData();
    return std::string(data.begin(), data.end());
}

TEST(FlashCommandsTest, ReadsHexAddressAndPrintsData)
{
    MockUart uart;
    platform::middleware::cli::UartConsole console(uart);
    ASSERT_TRUE(console.initialize());
    MockFlashStorage flash;
    flash.setReadData({0xDEU, 0xADU});
    app::console::commands::FlashCommands commands(console, flash);
    platform::middleware::cli::CommandRegistry registry;
    ASSERT_TRUE(commands.registerCommands(registry));

    ASSERT_TRUE(registry.dispatch("flash read 0x001234 2"));

    EXPECT_EQ(flash.readCallCount(), 1U);
    EXPECT_EQ(flash.lastReadAddress(), 0x1234U);
    EXPECT_NE(transmittedText(uart).find("0x00001234: DE AD"), std::string::npos);
}

TEST(FlashCommandsTest, WritesHexBytesWithoutImplicitErase)
{
    MockUart uart;
    platform::middleware::cli::UartConsole console(uart);
    ASSERT_TRUE(console.initialize());
    MockFlashStorage flash;
    app::console::commands::FlashCommands commands(console, flash);
    platform::middleware::cli::CommandRegistry registry;
    ASSERT_TRUE(commands.registerCommands(registry));

    ASSERT_TRUE(registry.dispatch("flash write 2000 a1B2c3"));

    EXPECT_EQ(flash.writeCallCount(), 1U);
    EXPECT_EQ(flash.lastWriteAddress(), 0x2000U);
    EXPECT_EQ(flash.writtenData(), (std::vector<std::uint8_t>{0xA1U, 0xB2U, 0xC3U}));
    EXPECT_EQ(flash.eraseCallCount(), 0U);
}

TEST(FlashCommandsTest, ErasesOnlyTheRequestedAlignedSector)
{
    MockUart uart;
    platform::middleware::cli::UartConsole console(uart);
    ASSERT_TRUE(console.initialize());
    MockFlashStorage flash;
    app::console::commands::FlashCommands commands(console, flash);
    platform::middleware::cli::CommandRegistry registry;
    ASSERT_TRUE(commands.registerCommands(registry));

    ASSERT_TRUE(registry.dispatch("flash erase 0x3000"));

    EXPECT_EQ(flash.eraseCallCount(), 1U);
    EXPECT_EQ(flash.lastEraseAddress(), 0x3000U);
    EXPECT_EQ(flash.lastEraseSize(), 4096U);
}

TEST(FlashCommandsTest, RejectsMalformedWriteWithoutCallingFlash)
{
    MockUart uart;
    platform::middleware::cli::UartConsole console(uart);
    ASSERT_TRUE(console.initialize());
    MockFlashStorage flash;
    app::console::commands::FlashCommands commands(console, flash);
    platform::middleware::cli::CommandRegistry registry;
    ASSERT_TRUE(commands.registerCommands(registry));

    ASSERT_TRUE(registry.dispatch("flash write 1000 ABC"));

    EXPECT_EQ(flash.writeCallCount(), 0U);
    EXPECT_NE(transmittedText(uart).find("flash write <hex-address> <hex-bytes>"),
              std::string::npos);
}

TEST(FlashCommandsTest, RejectsUnalignedEraseAddress)
{
    MockUart uart;
    platform::middleware::cli::UartConsole console(uart);
    ASSERT_TRUE(console.initialize());
    MockFlashStorage flash;
    app::console::commands::FlashCommands commands(console, flash);
    platform::middleware::cli::CommandRegistry registry;
    ASSERT_TRUE(commands.registerCommands(registry));

    ASSERT_TRUE(registry.dispatch("flash erase 3001"));

    EXPECT_EQ(flash.eraseCallCount(), 0U);
}

} // namespace