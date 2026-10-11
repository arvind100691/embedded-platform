#include <cstdint>

#include "platform/bsp/stm32f103_board/Board.hpp"
#include "platform/hal/IUart.hpp"
#include "platform/stm32f103/HardFaultHandler.hpp"

#include "stm32f1xx.h"

extern "C"
{
    extern std::uint32_t _estack;
    extern std::uint32_t _etext;
}

namespace
{

constexpr std::uint32_t kMaxStackTraceFrames = 16U;
constexpr std::uint32_t kExceptionFrameWords = 8U;
constexpr std::uint32_t kStackAlignmentFlag = 1U << 9U;

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

bool isValidStackFrame(std::uintptr_t stackFrame)
{
    const std::uintptr_t stackEnd = reinterpret_cast<std::uintptr_t>(&_estack);
    constexpr std::uintptr_t kExceptionFrameSize = kExceptionFrameWords * sizeof(std::uint32_t);

    return (stackFrame & (alignof(std::uint32_t) - 1U)) == 0U &&
           stackFrame >= static_cast<std::uintptr_t>(SRAM_BASE) &&
           stackFrame <= stackEnd - kExceptionFrameSize;
}

bool isThumbCodeAddress(std::uint32_t address)
{
    const std::uintptr_t codeAddress = static_cast<std::uintptr_t>(address & ~1U);
    const std::uintptr_t codeEnd = reinterpret_cast<std::uintptr_t>(&_etext);

    return (address & 1U) != 0U && codeAddress >= static_cast<std::uintptr_t>(FLASH_BASE) &&
           codeAddress < codeEnd;
}

bool followsThumbCall(std::uint32_t address)
{
    if (!isThumbCodeAddress(address))
    {
        return false;
    }

    const std::uintptr_t returnAddress = static_cast<std::uintptr_t>(address & ~1U);
    if (returnAddress < static_cast<std::uintptr_t>(FLASH_BASE) + 2U)
    {
        return false;
    }

    const auto* instructionBeforeReturn =
        reinterpret_cast<const std::uint16_t*>(returnAddress - sizeof(std::uint16_t));
    const std::uint16_t lastHalfword = *instructionBeforeReturn;

    // BLX (register) is a 16-bit call; BL immediate is a 32-bit call.
    if ((lastHalfword & 0xFF87U) == 0x4780U)
    {
        return true;
    }

    if (returnAddress < static_cast<std::uintptr_t>(FLASH_BASE) + 4U)
    {
        return false;
    }

    const auto* firstHalfword =
        reinterpret_cast<const std::uint16_t*>(returnAddress - 2U * sizeof(std::uint16_t));
    const std::uint16_t first = *firstHalfword;

    return (first & 0xF800U) == 0xF000U && (lastHalfword & 0xD000U) == 0xD000U;
}

void dumpStackTrace(platform::hal::IUart& uart, const std::uint32_t* stackFrame,
                    const platform::common::FaultRegisters& registers)
{
    writeString(uart, "Stack trace (best effort):\r\n");
    writeRegisterLine(uart, "#0 PC: ", registers.pc);
    writeRegisterLine(uart, "#1 LR: ", registers.lr);

    const std::uintptr_t stackAddress = reinterpret_cast<std::uintptr_t>(stackFrame);
    if (!isValidStackFrame(stackAddress))
    {
        return;
    }

    std::uintptr_t scanAddress = stackAddress + kExceptionFrameWords * sizeof(std::uint32_t);
    if ((stackFrame[7] & kStackAlignmentFlag) != 0U)
    {
        scanAddress += sizeof(std::uint32_t);
    }

    const std::uintptr_t stackEnd = reinterpret_cast<std::uintptr_t>(&_estack);
    std::uint32_t lastAddress = registers.lr & ~1U;
    std::uint32_t frame = 2U;
    while (scanAddress <= stackEnd - sizeof(std::uint32_t) && frame < kMaxStackTraceFrames)
    {
        const auto* word = reinterpret_cast<const std::uint32_t*>(scanAddress);
        const std::uint32_t returnAddress = *word;
        if (followsThumbCall(returnAddress) && (returnAddress & ~1U) != (registers.pc & ~1U) &&
            (returnAddress & ~1U) != lastAddress)
        {
            writeString(uart, "#");
            writeHex32(uart, frame);
            writeString(uart, " LR: 0x");
            writeHex32(uart, returnAddress);
            writeString(uart, "\r\n");
            lastAddress = returnAddress & ~1U;
            ++frame;
        }

        scanAddress += sizeof(std::uint32_t);
    }
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

extern "C" [[noreturn]] void HardFault_Handler_C(const std::uint32_t* stackFrame,
                                                 std::uint32_t exceptionReturn)
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
        writeRegisterLine(uart, "Frame: ",
                          static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(stackFrame)));
        writeRegisterLine(uart, "EXC_RETURN: ", exceptionReturn);
        const platform::common::FaultRegisters registers =
            platform::stm32f103::captureFaultRegisters(stackFrame);
        dumpFaultRegisters(uart, registers);
        dumpStackTrace(uart, stackFrame, registers);
    }

    while (true)
    {
        __asm volatile("nop");
    }
}

extern "C" __attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile("tst lr, #4\n"
                   "ite eq\n"
                   "mrseq r0, msp\n"
                   "mrsne r0, psp\n"
                   "mov r1, lr\n"
                   "b HardFault_Handler_C\n");
}
