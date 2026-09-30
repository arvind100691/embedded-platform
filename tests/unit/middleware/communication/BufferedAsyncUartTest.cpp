#include "platform/middleware/communication/BufferedAsyncUart.hpp"

#include <array>
#include <gtest/gtest.h>
#include <vector>

#include "MockAsyncUart.hpp"

namespace
{

using BufferedUart = platform::middleware::communication::BufferedAsyncUart<4U, 8U, 2U>;

} // namespace

TEST(SpscByteRingBufferTest, WrapsWithNonPowerOfTwoCapacity)
{
    platform::SpscByteRingBuffer<3U> buffer;
    constexpr std::array<platform::Byte, 3U> initial{'A', 'B', 'C'};
    constexpr std::array<platform::Byte, 2U> appended{'D', 'E'};

    ASSERT_EQ(buffer.write(initial.data(), initial.size()), initial.size());

    std::array<platform::Byte, 2U> first{};
    ASSERT_EQ(buffer.read(first.data(), first.size()), first.size());
    EXPECT_EQ(first[0], 'A');
    EXPECT_EQ(first[1], 'B');

    ASSERT_EQ(buffer.write(appended.data(), appended.size()), appended.size());

    std::array<platform::Byte, 3U> remaining{};
    ASSERT_EQ(buffer.read(remaining.data(), remaining.size()), remaining.size());
    EXPECT_EQ(remaining[0], 'C');
    EXPECT_EQ(remaining[1], 'D');
    EXPECT_EQ(remaining[2], 'E');
    EXPECT_TRUE(buffer.empty());
}

TEST(BufferedAsyncUartTest, StartArmsReceive)
{
    MockAsyncUart uart;
    BufferedUart bufferedUart(uart);

    EXPECT_TRUE(bufferedUart.start());
    EXPECT_TRUE(bufferedUart.running());
    EXPECT_TRUE(uart.receiveActive());
    EXPECT_EQ(uart.receiveCallCount(), 1U);
}

TEST(BufferedAsyncUartTest, ReceiveIsBufferedAndRearmedByPoll)
{
    MockAsyncUart uart;
    BufferedUart bufferedUart(uart);

    ASSERT_TRUE(bufferedUart.start());
    ASSERT_TRUE(uart.completeReceiveByte('A'));
    EXPECT_FALSE(uart.receiveActive());
    EXPECT_EQ(bufferedUart.available(), 1U);

    ASSERT_TRUE(bufferedUart.poll());
    EXPECT_TRUE(uart.receiveActive());
    EXPECT_EQ(uart.receiveCallCount(), 2U);

    std::array<platform::Byte, 1U> data{};
    const auto result = bufferedUart.read(data.data(), data.size());

    ASSERT_TRUE(result);
    ASSERT_NE(result.value(), nullptr);
    EXPECT_EQ(*result.value(), 1U);
    EXPECT_EQ(data[0], 'A');
}

TEST(BufferedAsyncUartTest, ReceiveOverflowIsReported)
{
    MockAsyncUart uart;
    BufferedUart bufferedUart(uart);

    ASSERT_TRUE(bufferedUart.start());
    for (const platform::Byte value : {'A', 'B', 'C', 'D', 'E'})
    {
        ASSERT_TRUE(uart.completeReceiveByte(value));
        ASSERT_TRUE(bufferedUart.poll());
    }

    EXPECT_EQ(bufferedUart.available(), 4U);
    EXPECT_TRUE(bufferedUart.rxOverflowed());
}

TEST(BufferedAsyncUartTest, ReceiveErrorIsReportedAndCanBeCleared)
{
    MockAsyncUart uart;
    BufferedUart bufferedUart(uart);

    ASSERT_TRUE(bufferedUart.start());
    uart.completeReceive(platform::hal::UartTransferStatus::Error, 0U);

    EXPECT_TRUE(bufferedUart.rxError());
    EXPECT_FALSE(bufferedUart.poll());

    bufferedUart.clearErrors();
    EXPECT_TRUE(bufferedUart.poll());
    EXPECT_TRUE(uart.receiveActive());
}

TEST(BufferedAsyncUartTest, TxIsChunkedAndQueued)
{
    MockAsyncUart uart;
    BufferedUart bufferedUart(uart);

    ASSERT_TRUE(bufferedUart.start());
    constexpr std::array<platform::Byte, 5U> data{'A', 'B', 'C', 'D', 'E'};

    const auto result = bufferedUart.write(data.data(), data.size());
    ASSERT_TRUE(result);
    ASSERT_NE(result.value(), nullptr);
    EXPECT_EQ(*result.value(), data.size());
    EXPECT_EQ(uart.transmittedData(), std::vector<std::uint8_t>({'A', 'B'}));

    uart.completeTransmit(platform::hal::UartTransferStatus::Completed, 2U);
    ASSERT_TRUE(bufferedUart.poll());
    EXPECT_EQ(uart.transmittedData(), std::vector<std::uint8_t>({'C', 'D'}));

    uart.completeTransmit(platform::hal::UartTransferStatus::Completed, 2U);
    ASSERT_TRUE(bufferedUart.poll());
    EXPECT_EQ(uart.transmittedData(), std::vector<std::uint8_t>({'E'}));
}

TEST(BufferedAsyncUartTest, TxBufferFullIsRejected)
{
    MockAsyncUart uart;
    BufferedUart bufferedUart(uart);

    ASSERT_TRUE(bufferedUart.start());
    constexpr std::array<platform::Byte, 9U> data{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'};

    const auto result = bufferedUart.write(data.data(), data.size());
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::BufferFull);
}

TEST(BufferedAsyncUartTest, PollTreatsBusyTransmitStartAsRetryable)
{
    MockAsyncUart uart;
    BufferedUart bufferedUart(uart);

    ASSERT_TRUE(bufferedUart.start());
    uart.setTransmitStartFailure(platform::ErrorCode::Busy);

    constexpr std::array<platform::Byte, 2U> data{'A', 'B'};
    ASSERT_TRUE(bufferedUart.write(data.data(), data.size()));

    EXPECT_FALSE(uart.transmitActive());
    EXPECT_TRUE(bufferedUart.poll());
    EXPECT_FALSE(bufferedUart.txError());
    EXPECT_EQ(bufferedUart.freeTxSpace(), 6U);
}

TEST(BufferedAsyncUartTest, TxErrorStopsFurtherTransmission)
{
    MockAsyncUart uart;
    BufferedUart bufferedUart(uart);

    ASSERT_TRUE(bufferedUart.start());
    constexpr std::array<platform::Byte, 2U> data{'A', 'B'};
    ASSERT_TRUE(bufferedUart.write(data.data(), data.size()));

    uart.completeTransmit(platform::hal::UartTransferStatus::Error, 0U);
    EXPECT_TRUE(bufferedUart.txError());
    EXPECT_FALSE(bufferedUart.poll());
}

TEST(BufferedAsyncUartTest, StopCancelsActiveTransfers)
{
    MockAsyncUart uart;
    BufferedUart bufferedUart(uart);

    ASSERT_TRUE(bufferedUart.start());
    constexpr std::array<platform::Byte, 2U> data{'A', 'B'};
    ASSERT_TRUE(bufferedUart.write(data.data(), data.size()));

    ASSERT_TRUE(uart.receiveActive());
    ASSERT_TRUE(uart.transmitActive());
    EXPECT_TRUE(bufferedUart.stop());
    EXPECT_FALSE(bufferedUart.running());
    EXPECT_EQ(uart.cancelReceiveCallCount(), 1U);
    EXPECT_EQ(uart.cancelTransmitCallCount(), 1U);
}