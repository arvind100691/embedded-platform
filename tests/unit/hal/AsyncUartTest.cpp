#include <cstdint>
#include <gtest/gtest.h>

#include "MockAsyncUart.hpp"

namespace
{

struct CallbackState
{
    int callCount{0};
    platform::hal::UartTransferStatus status{platform::hal::UartTransferStatus::Error};
    std::size_t transferred{0U};
};

void callback(platform::hal::UartTransferStatus status, std::size_t transferredBytes, void* context)
{
    auto* state = static_cast<CallbackState*>(context);
    ++state->callCount;
    state->status = status;
    state->transferred = transferredBytes;
}

} // namespace

TEST(MockAsyncUartTest, TransmitCompletionInvokesCallback)
{
    MockAsyncUart uart;
    CallbackState state{};
    constexpr std::uint8_t data[]{'U', 'A'};

    EXPECT_TRUE(uart.transmitAsync(data, sizeof(data), callback, &state));
    EXPECT_TRUE(uart.transmitActive());

    uart.completeTransmit(platform::hal::UartTransferStatus::Completed, sizeof(data));

    EXPECT_FALSE(uart.transmitActive());
    EXPECT_EQ(state.callCount, 1);
    EXPECT_EQ(state.status, platform::hal::UartTransferStatus::Completed);
    EXPECT_EQ(state.transferred, sizeof(data));
}

TEST(MockAsyncUartTest, ReceiveCompletionInvokesCallback)
{
    MockAsyncUart uart;
    CallbackState state{};
    std::uint8_t buffer[4]{};

    EXPECT_TRUE(uart.receiveAsync(buffer, sizeof(buffer), callback, &state));
    EXPECT_TRUE(uart.receiveActive());

    uart.completeReceive(platform::hal::UartTransferStatus::Completed, sizeof(buffer));

    EXPECT_FALSE(uart.receiveActive());
    EXPECT_EQ(state.callCount, 1);
    EXPECT_EQ(state.status, platform::hal::UartTransferStatus::Completed);
    EXPECT_EQ(state.transferred, sizeof(buffer));
}

TEST(MockAsyncUartTest, BusyTransmitIsRejected)
{
    MockAsyncUart uart;
    CallbackState state{};
    constexpr std::uint8_t data[]{'A'};

    EXPECT_TRUE(uart.transmitAsync(data, 1U, callback, &state));
    EXPECT_FALSE(uart.transmitAsync(data, 1U, callback, &state));
    EXPECT_EQ(uart.transmitCallCount(), 2U);
}

TEST(MockAsyncUartTest, CancelTransmitInvokesAbortedCallback)
{
    MockAsyncUart uart;
    CallbackState state{};
    constexpr std::uint8_t data[]{'A', 'B', 'C'};

    EXPECT_TRUE(uart.transmitAsync(data, sizeof(data), callback, &state));
    uart.setTransmitTransferredBeforeCancel(1U);

    EXPECT_TRUE(uart.cancelTransmit());

    EXPECT_EQ(state.callCount, 1);
    EXPECT_EQ(state.status, platform::hal::UartTransferStatus::Aborted);
    EXPECT_EQ(state.transferred, 1U);
    EXPECT_FALSE(uart.transmitActive());
}

TEST(MockAsyncUartTest, StartFailureDoesNotActivateTransfer)
{
    MockAsyncUart uart;
    CallbackState state{};
    constexpr std::uint8_t data[]{'A'};

    uart.setTransmitStartFailure(platform::ErrorCode::Busy);

    const auto result = uart.transmitAsync(data, 1U, callback, &state);

    EXPECT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::Busy);
    EXPECT_FALSE(uart.transmitActive());
    EXPECT_EQ(state.callCount, 0);
}

TEST(MockAsyncUartTest, NullCallbackIsRejected)
{
    MockAsyncUart uart;
    constexpr std::uint8_t data[]{'A'};

    const auto result = uart.transmitAsync(data, 1U, nullptr, nullptr);

    EXPECT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::InvalidArgument);
}
