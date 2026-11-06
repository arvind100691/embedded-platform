#include "platform/bsp/stm32f103_board/Board.hpp"
#include "platform/drivers/max30102/Max30102.hpp"
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
                                    "I2C1: SCL PB6, SDA PB7\r\n"
                                    "MAX30102 raw stream starts at boot; use 'sensor off' to stop\r\n"
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

    platform::hal::I2cConfig i2cConfig{};
    i2cConfig.frequencyHz = 100'000U;
    i2cConfig.addressMode = platform::hal::I2cAddressMode::SevenBit;
    platform::hal::II2c& i2c = platform::bsp::stm32f103_board::i2c1();
    if (!i2c.configure(i2cConfig))
    {
        static_cast<void>(console.write("I2C1 configuration failed\r\n"));
        return 1;
    }
    platform::drivers::max30102::Max30102 max30102(i2c);

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

    app::Application application(platform::bsp::stm32f103_board::statusLed(), console, flash,
                                 &max30102);

    return application.run();
}