#include "platform/bsp/stm32f103_board/Board.hpp"
#include "platform/middleware/cli/IConsole.hpp"
#include "platform/middleware/cli/UartConsole.hpp"

#include "app/Application.hpp"

namespace
{

void printStartupMessage(platform::middleware::cli::IConsole& console)
{
    constexpr char kBootMessage[] = "Embedded Platform boot\r\n"
                                    "Target: STM32F103\r\n"
                                    "Board: stm32f103_board\r\n"
                                    "Console UART: USART1 (PA9 TX, PA10 RX)\r\n"
                                    "Status LED: PC13\r\n";
    (void)console.write(std::string_view(kBootMessage, sizeof(kBootMessage) - 1U));
}

} // namespace

int main()
{
    platform::bsp::stm32f103_board::init();

    platform::middleware::cli::UartConsole console(platform::bsp::stm32f103_board::consoleUart());
    if (!console.initialize())
    {
        return 1;
    }
    printStartupMessage(console);

    app::Application application(platform::bsp::stm32f103_board::statusLed(), console);

    return application.run();
}