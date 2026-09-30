#include <array>
#include <cstdint>

#include "platform/bsp/stm32f103_board/Board.hpp"
#include "platform/hal/IAsyncUart.hpp"
#include "platform/hal/IUart.hpp"

#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"

enum class TestStatus : std::uint32_t
{
    NotStarted = 0x0000,

    UartConfigured = 0x0001,
    ReceiveStarted = 0x0002,
    TransmitStarted = 0x0003,
    ReceivePassed = 0x0004,
    TransmitPassed = 0x0005,
    DataMatchPassed = 0x0006,

    Passed = 0xAA55,

    UartConfigureFailed = 0x1001,
    ReceiveStartFailed = 0x1002,
    TransmitStartFailed = 0x1003,
    ReceiveFailed = 0x1004,
    TransmitFailed = 0x1005,
    ReceiveSizeFailed = 0x1006,
    TransmitSizeFailed = 0x1007,
    DataMismatch = 0x1008,
    Timeout = 0x1009
};

struct TransferState
{
    volatile bool callbackCalled{false};
    volatile platform::hal::UartTransferStatus status{platform::hal::UartTransferStatus::Error};
    volatile std::size_t transferredBytes{0U};
};

extern "C"
{
    volatile TestStatus g_uart_async_test_status = TestStatus::NotStarted;
    volatile std::uint32_t g_uart_async_tx_callback_count = 0U;
    volatile std::uint32_t g_uart_async_rx_callback_count = 0U;
    volatile std::uint32_t g_uart_async_tx_transferred = 0U;
    volatile std::uint32_t g_uart_async_rx_transferred = 0U;
}

static void txCallback(platform::hal::UartTransferStatus status, std::size_t transferredBytes,
                       void* context)
{
    auto* state = static_cast<TransferState*>(context);
    state->status = status;
    state->transferredBytes = transferredBytes;
    state->callbackCalled = true;

    ++g_uart_async_tx_callback_count;
    g_uart_async_tx_transferred = static_cast<std::uint32_t>(transferredBytes);
}

static void rxCallback(platform::hal::UartTransferStatus status, std::size_t transferredBytes,
                       void* context)
{
    auto* state = static_cast<TransferState*>(context);
    state->status = status;
    state->transferredBytes = transferredBytes;
    state->callbackCalled = true;

    ++g_uart_async_rx_callback_count;
    g_uart_async_rx_transferred = static_cast<std::uint32_t>(transferredBytes);
}

[[noreturn]] static void testFailure(TestStatus status)
{
    g_uart_async_test_status = status;
    __disable_irq();

    while (true)
    {
        __asm volatile("bkpt #0");
    }
}

static bool waitForCompletion(const TransferState& state, std::uint32_t timeoutMs)
{
    const std::uint32_t start = HAL_GetTick();

    while (!state.callbackCalled)
    {
        if ((HAL_GetTick() - start) >= timeoutMs)
        {
            return false;
        }
    }

    return true;
}

int main()
{
    /*
     * Hardware connection:
     *
     *     PA9  USART1_TX  --------  PA10  USART1_RX
     *
     * Receive is started before transmit so the RX interrupt is already
     * armed when the first byte is placed on the TX pin.
     */
    platform::bsp::stm32f103_board::init();

    platform::hal::IUart& uart = platform::bsp::stm32f103_board::consoleUart();
    platform::hal::IAsyncUart& asyncUart = platform::bsp::stm32f103_board::consoleAsyncUart();

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

    g_uart_async_test_status = TestStatus::UartConfigured;

    constexpr std::array<std::uint8_t, 8U> kTestData{'U', 'A', 'R', 'T', '1', '2', '3', '\n'};

    std::array<std::uint8_t, kTestData.size()> received{};
    TransferState rxState{};
    TransferState txState{};

    const auto receiveResult =
        asyncUart.receiveAsync(received.data(), received.size(), rxCallback, &rxState);

    if (!receiveResult)
    {
        testFailure(TestStatus::ReceiveStartFailed);
    }

    g_uart_async_test_status = TestStatus::ReceiveStarted;

    const auto transmitResult =
        asyncUart.transmitAsync(kTestData.data(), kTestData.size(), txCallback, &txState);

    if (!transmitResult)
    {
        testFailure(TestStatus::TransmitStartFailed);
    }

    g_uart_async_test_status = TestStatus::TransmitStarted;

    if (!waitForCompletion(txState, 1000U))
    {
        testFailure(TestStatus::Timeout);
    }

    if (txState.status != platform::hal::UartTransferStatus::Completed)
    {
        testFailure(TestStatus::TransmitFailed);
    }

    if (txState.transferredBytes != kTestData.size())
    {
        testFailure(TestStatus::TransmitSizeFailed);
    }

    g_uart_async_test_status = TestStatus::TransmitPassed;

    if (!waitForCompletion(rxState, 1000U))
    {
        testFailure(TestStatus::Timeout);
    }

    if (rxState.status != platform::hal::UartTransferStatus::Completed)
    {
        testFailure(TestStatus::ReceiveFailed);
    }

    if (rxState.transferredBytes != kTestData.size())
    {
        testFailure(TestStatus::ReceiveSizeFailed);
    }

    g_uart_async_test_status = TestStatus::ReceivePassed;

    for (std::size_t index = 0U; index < kTestData.size(); ++index)
    {
        if (received[index] != kTestData[index])
        {
            testFailure(TestStatus::DataMismatch);
        }
    }

    g_uart_async_test_status = TestStatus::DataMatchPassed;
    g_uart_async_test_status = TestStatus::Passed;

    __disable_irq();

    while (true)
    {
        __asm volatile("nop");
    }
}
