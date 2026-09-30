#include "platform/bsp/stm32f103_board/Board.hpp"
#include "platform/hal/IUart.hpp"
#include "platform/stm32f103/HardFaultHandler.hpp"

#include "stm32f1xx.h"

namespace
{

void writeString(platform::hal::IUart& uart, const char* text)
{
    if (text == nullptr)
    {
        return;
    }

    while (*text != '\0')
    {
        const std::uint8_t byte = static_cast<std::uint8_t>(*text++);
        (void)uart.transmit(&byte, 1U, 100U);
    }
}

void writeHex32(platform::hal::IUart& uart, std::uint32_t value)
{
    static constexpr char kHexDigits[] = "0123456789ABCDEF";
    char buffer[9];

    for (int index = 7; index >= 0; --index)
    {
        const std::uint8_t nibble =
            static_cast<std::uint8_t>((value >> (4U * static_cast<unsigned int>(index))) & 0xFU);
        buffer[7 - index] = kHexDigits[nibble];
    }
    buffer[8] = '\0';

    writeString(uart, buffer);
}

void writeRegisterLine(platform::hal::IUart& uart, const char* label, std::uint32_t value)
{
    writeString(uart, label);
    writeString(uart, "0x");
    writeHex32(uart, value);
    writeString(uart, "\r\n");
}

void dumpFaultRegisters(platform::hal::IUart& uart,
                        const platform::common::FaultRegisters& registers)
{
    writeString(uart, "HardFault detected\r\n");
    writeRegisterLine(uart, "R0: ", registers.r0);
    writeRegisterLine(uart, "R1: ", registers.r1);
    writeRegisterLine(uart, "R2: ", registers.r2);
    writeRegisterLine(uart, "R3: ", registers.r3);
    writeRegisterLine(uart, "R12: ", registers.r12);
    writeRegisterLine(uart, "LR: ", registers.lr);
    writeRegisterLine(uart, "PC: ", registers.pc);
    writeRegisterLine(uart, "PSR: ", registers.psr);
    writeRegisterLine(uart, "CFSR: ", registers.cfsr);
    writeRegisterLine(uart, "HFSR: ", registers.hfsr);
    writeRegisterLine(uart, "DFSR: ", registers.dfsr);
    writeRegisterLine(uart, "AFSR: ", registers.afsr);
    writeRegisterLine(uart, "MMFAR: ", registers.mmfar);
    writeRegisterLine(uart, "BFAR: ", registers.bfar);
    writeRegisterLine(uart, "SHCSR: ", registers.shcsr);
}

} // namespace

extern "C" void HardFault_Handler(void)
{
    __disable_irq();

    platform::hal::IUart& uart = platform::bsp::stm32f103_board::consoleUart();

    platform::hal::UartConfig config{};
    config.baudRate = 115200U;
    config.dataBits = 8U;
    config.parity = platform::hal::UartParity::None;
    config.stopBits = platform::hal::UartStopBits::One;
    config.flowControl = platform::hal::UartFlowControl::None;

    if (uart.configure(config))
    {
        dumpFaultRegisters(uart, platform::stm32f103::captureFaultRegisters());
    }

    while (true)
    {
        __asm volatile("nop");
    }
}
