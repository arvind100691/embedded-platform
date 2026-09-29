#include <array>
#include <cstdint>

#include "platform/bsp/stm32f103_board/Board.hpp"
#include "platform/common/ErrorCode.hpp"
#include "platform/common/Types.hpp"
#include "platform/hal/IUart.hpp"
#include "platform/stm32f103/Stm32Uart.hpp"

#include "stm32f1xx.h"

namespace
{

enum class TestStatus : std::uint32_t
{
    NotStarted = 0x0000,

    UartConfigured = 0x0001,
    ByteTestStarted = 0x0002,
    TransmitPassed = 0x0003,
    ReceivePassed = 0x0004,
    DataMatchPassed = 0x0005,

    Passed = 0xAA55,

    UartConfigureFailed = 0x1001,
    TransmitFailed = 0x1002,
    ReceiveFailed = 0x1003,
    ReceiveSizeFailed = 0x1004,
    DataMismatch = 0x1005
};

volatile TestStatus g_uart_test_status = TestStatus::NotStarted;
volatile std::uint32_t g_uart_test_byte_count = 0U;

[[noreturn]] void testFailure(TestStatus status)
{
    g_uart_test_status = status;

    __disable_irq();

    while (true)
    {
        __asm volatile("bkpt #0");
    }
}

void smallDelay()
{
    for (volatile std::uint32_t i = 0U; i < 1000U; ++i)
    {
        __asm volatile("nop");
    }
}

} // namespace

int main()
{
    /*
     * Hardware connection:
     *
     *     PA9  USART1_TX  --------  PA10  USART1_RX
     *
     * This is a physical UART loopback test.
     */
    platform::bsp::stm32f103_board::init();

    platform::hal::IUart& uart = platform::bsp::stm32f103_board::consoleUart();

    platform::hal::UartConfig config{};
    config.baudRate = 115200U;
    config.dataBits = 8U;
    config.parity = platform::hal::UartParity::None;
    config.stopBits = platform::hal::UartStopBits::One;
    config.flowControl = platform::hal::UartFlowControl::None;

    if (!uart.configure(config))
    {
        testFailure(TestStatus::UartConfigureFailed);
    }

    g_uart_test_status = TestStatus::UartConfigured;

    constexpr std::array<std::uint8_t, 8U> kTestData{'U', 'A', 'R', 'T', '1', '2', '3', '\n'};

    for (const std::uint8_t value : kTestData)
    {
        g_uart_test_status = TestStatus::ByteTestStarted;

        const auto transmitResult = uart.transmit(&value, 1U, 100U);

        if (!transmitResult)
        {
            testFailure(TestStatus::TransmitFailed);
        }

        g_uart_test_status = TestStatus::TransmitPassed;

        smallDelay();

        std::uint8_t received = 0U;

        const auto receiveResult = uart.receive(&received, 1U, 100U);

        if (!receiveResult)
        {
            testFailure(TestStatus::ReceiveFailed);
        }

        if (receiveResult.value() == nullptr || *receiveResult.value() != 1U)
        {
            testFailure(TestStatus::ReceiveSizeFailed);
        }

        g_uart_test_status = TestStatus::ReceivePassed;

        if (received != value)
        {
            testFailure(TestStatus::DataMismatch);
        }

        g_uart_test_status = TestStatus::DataMatchPassed;
        ++g_uart_test_byte_count;
    }

    g_uart_test_status = TestStatus::Passed;

    __disable_irq();

    while (true)
    {
        __asm volatile("nop");
    }
}
