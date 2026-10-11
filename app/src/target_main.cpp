#include "platform/bsp/stm32f103_board/Board.hpp"
#include "platform/hal/ISpi.hpp"
#include "platform/middleware/cli/IConsole.hpp"
#include "platform/middleware/cli/UartConsole.hpp"
#include "platform/middleware/storage/W25Q128.hpp"

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

    platform::hal::SpiConfig spiConfig{};
    spiConfig.frequencyHz = 1'000'000U;
    spiConfig.mode = platform::hal::SpiMode::Mode0;
    spiConfig.bitOrder = platform::hal::SpiBitOrder::MsbFirst;
    spiConfig.dataBits = 8U;
    platform::hal::ISpi& spi = platform::bsp::stm32f103_board::spi1();
    if (!spi.configure(spiConfig))
    {
        static_cast<void>(console.write("SPI configuration failed\r\n"));
        return 1;
    }

    platform::middleware::storage::W25Q128 flash(spi,
                                                 platform::bsp::stm32f103_board::spi1ChipSelect());
    if (!flash.initialize())
    {
        static_cast<void>(console.write("W25Q128 initialization failed\r\n"));
        return 1;
    }

    app::Application application(platform::bsp::stm32f103_board::statusLed(), console, flash);

    return application.run();
}