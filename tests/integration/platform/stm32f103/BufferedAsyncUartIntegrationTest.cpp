#include <array>
#include <cstdint>

#include "platform/bsp/stm32f103_board/Board.hpp"
#include "platform/middleware/communication/BufferedAsyncUart.hpp"

#include "stm32f1xx.h"

enum class TestStatus : std::uint32_t
{
    NotStarted = 0x0000,
    UartConfigured = 0x0001,
    BufferStarted = 0x0002,
    DataQueued = 0x0003,
    DataReceived = 0x0004,

    Passed = 0xAA55,

    ConfigureFailed = 0x1001,
    StartFailed = 0x1002,
    WriteFailed = 0x1003,
    PollFailed = 0x1004,
    Timeout = 0x1005,
    ReceiveFailed = 0x1006,
    DataMismatch = 0x1007,
    ReceiveOverflow = 0x1008
};

extern "C"
{
    volatile TestStatus g_buffered_uart_test_status = TestStatus::NotStarted;
    volatile std::uint32_t g_buffered_uart_received = 0U;
}

[[noreturn]] static void testFailure(TestStatus status)
{
    g_buffered_uart_test_status = status;
    __disable_irq();

    while (true)
    {
        __asm volatile("bkpt #0");
    }
}

int main()
{
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
        testFailure(TestStatus::ConfigureFailed);
    }

    g_buffered_uart_test_status = TestStatus::UartConfigured;

    using BufferedUart = platform::middleware::communication::BufferedAsyncUart<32U, 64U, 8U>;
    platform::hal::IAsyncUart& asyncUart = platform::bsp::stm32f103_board::consoleAsyncUart();
    BufferedUart bufferedUart(asyncUart);

    if (!bufferedUart.start())
    {
        testFailure(TestStatus::StartFailed);
    }

    g_buffered_uart_test_status = TestStatus::BufferStarted;

    constexpr std::array<std::uint8_t, 32U> kTestData{
        'B', 'U', 'F', 'F', 'E', 'R', 'E', 'D', ' ', 'U', 'A', 'R', 'T', ' ',  'T',  'E',
        'S', 'T', ' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '\r', '\n', '!'};

    std::array<std::uint8_t, kTestData.size()> received{};
    std::size_t receivedCount = 0U;

    if (!bufferedUart.write(kTestData.data(), kTestData.size()))
    {
        testFailure(TestStatus::WriteFailed);
    }

    g_buffered_uart_test_status = TestStatus::DataQueued;
    const std::uint32_t startTick = HAL_GetTick();

    while (receivedCount < kTestData.size())
    {
        if (!bufferedUart.poll())
        {
            testFailure(TestStatus::PollFailed);
        }

        if (bufferedUart.rxOverflowed())
        {
            testFailure(TestStatus::ReceiveOverflow);
        }

        if (bufferedUart.available() > 0U)
        {
            const std::size_t remaining = kTestData.size() - receivedCount;
            const std::size_t requestSize = (remaining < 8U) ? remaining : 8U;
            const auto result = bufferedUart.read(received.data() + receivedCount, requestSize);

            if (!result || result.value() == nullptr)
            {
                testFailure(TestStatus::ReceiveFailed);
            }

            receivedCount += *result.value();
            g_buffered_uart_received = static_cast<std::uint32_t>(receivedCount);

            if (receivedCount == kTestData.size())
            {
                g_buffered_uart_test_status = TestStatus::DataReceived;
            }
        }

        if ((HAL_GetTick() - startTick) >= 2000U)
        {
            testFailure(TestStatus::Timeout);
        }
    }

    for (std::size_t index = 0U; index < kTestData.size(); ++index)
    {
        if (received[index] != kTestData[index])
        {
            testFailure(TestStatus::DataMismatch);
        }
    }

    g_buffered_uart_test_status = TestStatus::Passed;
    __disable_irq();

    while (true)
    {
        __asm volatile("nop");
    }
}