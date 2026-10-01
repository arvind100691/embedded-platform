#include "app/console/commands/LedCommands.hpp"

#include <gtest/gtest.h>

#include "platform/middleware/cli/UartConsole.hpp"

#include "MockUart.hpp"

namespace
{

TEST(LedCommandsTest, RegistersAndExecutesBlinkCommands)
{
    MockUart uart;
    platform::middleware::cli::UartConsole console(uart);
    ASSERT_TRUE(console.initialize());

    app::console::commands::LedCommands ledCommands(console);
    platform::middleware::cli::CommandRegistry registry;
    ASSERT_TRUE(ledCommands.registerCommands(registry));

    EXPECT_TRUE(ledCommands.blinkingEnabled());
    ASSERT_TRUE(registry.dispatch("blink off"));
    EXPECT_FALSE(ledCommands.blinkingEnabled());
    ASSERT_TRUE(registry.dispatch("blink on"));
    EXPECT_TRUE(ledCommands.blinkingEnabled());
    EXPECT_EQ(registry.dispatch("other").error(), platform::ErrorCode::NotFound);
}

} // namespace