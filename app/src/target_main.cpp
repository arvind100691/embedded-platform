#include "platform/bsp/stm32f103_board/Board.hpp"
#include "platform/hal/IUart.hpp"

#include "app/Application.hpp"

namespace
{

void printStartupMessage()
{
    platform::hal::IUart& uart = platform::bsp::stm32f103_board::consoleUart();

    platform::hal::UartConfig config{};
    config.baudRate = 115200U;
    config.dataBits = 8U;
    config.parity = platform::hal::UartParity::None;
    config.stopBits = platform::hal::UartStopBits::One;
    config.flowControl = platform::hal::UartFlowControl::None;

    if (!uart.configure(config))
    {
        return;
    }

    constexpr char kBootMessage[] = "Embedded Platform boot\r\n"
                                    "Target: STM32F103\r\n"
                                    "Board: stm32f103_board\r\n"
                                    "Console UART: USART1 (PA9 TX, PA10 RX)\r\n"
                                    "Status LED: PC13\r\n";
    (void)uart.transmit(reinterpret_cast<const std::uint8_t*>(kBootMessage),
                        sizeof(kBootMessage) - 1U, 100U);
}

} // namespace

int main()
{
    platform::bsp::stm32f103_board::init();
    printStartupMessage();

    app::Application application(platform::bsp::stm32f103_board::statusLed());

    return application.run();
}