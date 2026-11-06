#include <gtest/gtest.h>

#include <vector>

#include "platform/drivers/max30102/Max30102.hpp"
#include "MockI2c.hpp"

namespace
{

constexpr std::uint8_t kPartIdRegister{0xFFU};
constexpr std::uint8_t kExpectedPartId{0x15U};
constexpr std::uint8_t kFifoWritePointerRegister{0x04U};
constexpr std::uint8_t kFifoReadPointerRegister{0x06U};

void initializeSensor(MockI2c& i2c,
                       platform::drivers::max30102::Max30102& sensor)
{
    i2c.setRegister(kPartIdRegister, kExpectedPartId);
    ASSERT_TRUE(sensor.begin());
    for (std::size_t poll = 0U; poll < 13U; ++poll)
    {
        ASSERT_TRUE(sensor.poll());
    }
    ASSERT_TRUE(sensor.isReady());
}

} // namespace

TEST(Max30102Test, BeginDefersI2cAccessUntilPoll)
{
    MockI2c i2c;
    platform::drivers::max30102::Max30102 sensor(i2c);

    const auto result = sensor.begin();

    ASSERT_TRUE(result);
    EXPECT_TRUE(i2c.writeReadCalls().empty());
    EXPECT_FALSE(sensor.isReady());
}

TEST(Max30102Test, PollReadsPartIdFromSevenBitAddress)
{
    MockI2c i2c;
    i2c.setRegister(kPartIdRegister, kExpectedPartId);
    platform::drivers::max30102::Max30102 sensor(i2c);
    ASSERT_TRUE(sensor.begin());

    const auto result = sensor.poll();

    ASSERT_TRUE(result);
    ASSERT_EQ(i2c.writeReadCalls().size(), 1U);
    EXPECT_EQ(i2c.writeReadCalls()[0].address, 0x57U);
    EXPECT_EQ(i2c.writeReadCalls()[0].txData, std::vector<std::uint8_t>{kPartIdRegister});
    EXPECT_EQ(i2c.writeReadCalls()[0].rxSize, 1U);
    EXPECT_FALSE(sensor.isReady());
}

TEST(Max30102Test, InitializationWaitsForResetAndConfiguresDevice)
{
    constexpr std::uint8_t kFifoConfigurationRegister{0x08U};
    constexpr std::uint8_t kModeConfigurationRegister{0x09U};
    constexpr std::uint8_t kSpo2ConfigurationRegister{0x0AU};
    constexpr std::uint8_t kRedLedCurrentRegister{0x0CU};
    constexpr std::uint8_t kInfraredLedCurrentRegister{0x0DU};
    constexpr std::uint8_t kFifoWritePointerRegister{0x04U};
    constexpr std::uint8_t kFifoOverflowCounterRegister{0x05U};
    constexpr std::uint8_t kFifoReadPointerRegister{0x06U};
    constexpr std::size_t kTransactionsBeforeReady{13U};
    constexpr std::size_t kResetPendingPolls{2U};

    MockI2c i2c;
    i2c.setRegister(kPartIdRegister, kExpectedPartId);
    i2c.setResetPendingReads(kResetPendingPolls);
    platform::drivers::max30102::Max30102Config config{};
    config.redLedCurrent = 0x31U;
    config.infraredLedCurrent = 0x42U;
    platform::drivers::max30102::Max30102 sensor(i2c, config);
    ASSERT_TRUE(sensor.begin());

    for (std::size_t poll = 0U; poll < kTransactionsBeforeReady; ++poll)
    {
        const auto previousTransactionCount =
            i2c.writeCalls().size() + i2c.writeReadCalls().size();
        const auto result = sensor.poll();
        ASSERT_TRUE(result);
        EXPECT_EQ(i2c.writeCalls().size() + i2c.writeReadCalls().size(),
                  previousTransactionCount + 1U);
        if (poll < 4U)
        {
            EXPECT_FALSE(sensor.isReady());
        }
    }

    ASSERT_TRUE(sensor.isReady());
    ASSERT_EQ(i2c.writeCalls().size(), 9U);
    EXPECT_EQ(i2c.writeCalls()[0].data,
              (std::vector<std::uint8_t>{kModeConfigurationRegister, 0x40U}));
    EXPECT_EQ(i2c.writeCalls()[1].data,
              (std::vector<std::uint8_t>{kFifoConfigurationRegister, 0x10U}));
    EXPECT_EQ(i2c.writeCalls()[2].data,
              (std::vector<std::uint8_t>{kModeConfigurationRegister, 0x03U}));
    EXPECT_EQ(i2c.writeCalls()[3].data,
              (std::vector<std::uint8_t>{kSpo2ConfigurationRegister, 0x07U}));
    EXPECT_EQ(i2c.writeCalls()[4].data,
              (std::vector<std::uint8_t>{kRedLedCurrentRegister, config.redLedCurrent}));
    EXPECT_EQ(i2c.writeCalls()[5].data,
              (std::vector<std::uint8_t>{kInfraredLedCurrentRegister, config.infraredLedCurrent}));
    EXPECT_EQ(i2c.writeCalls()[6].data,
              (std::vector<std::uint8_t>{kFifoWritePointerRegister, 0U}));
    EXPECT_EQ(i2c.writeCalls()[7].data,
              (std::vector<std::uint8_t>{kFifoOverflowCounterRegister, 0U}));
    EXPECT_EQ(i2c.writeCalls()[8].data,
              (std::vector<std::uint8_t>{kFifoReadPointerRegister, 0U}));
}

TEST(Max30102Test, InitializationWriteErrorIsPropagated)
{
    MockI2c i2c;
    i2c.setRegister(kPartIdRegister, kExpectedPartId);
    platform::drivers::max30102::Max30102 sensor(i2c);
    ASSERT_TRUE(sensor.begin());
    ASSERT_TRUE(sensor.poll());
    i2c.setWriteFailure(platform::ErrorCode::CommunicationError);

    const auto result = sensor.poll();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::CommunicationError);
    EXPECT_FALSE(sensor.isReady());
}

TEST(Max30102Test, WrongPartIdReturnsNotFound)
{
    MockI2c i2c;
    i2c.setRegister(kPartIdRegister, 0x00U);
    platform::drivers::max30102::Max30102 sensor(i2c);
    ASSERT_TRUE(sensor.begin());

    const auto result = sensor.poll();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::NotFound);
}

TEST(Max30102Test, I2cReadErrorIsPropagated)
{
    MockI2c i2c;
    i2c.setWriteReadFailure(platform::ErrorCode::Timeout);
    platform::drivers::max30102::Max30102 sensor(i2c);
    ASSERT_TRUE(sensor.begin());

    const auto result = sensor.poll();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::Timeout);
}

TEST(Max30102Test, BusyDuringInitializationCanBeRetried)
{
    MockI2c i2c;
    i2c.setRegister(kPartIdRegister, kExpectedPartId);
    i2c.setWriteReadFailure(platform::ErrorCode::Busy);
    platform::drivers::max30102::Max30102 sensor(i2c);
    ASSERT_TRUE(sensor.begin());

    const auto busyResult = sensor.poll();
    ASSERT_FALSE(busyResult);
    EXPECT_EQ(busyResult.error(), platform::ErrorCode::Busy);
    EXPECT_FALSE(sensor.isReady());

    i2c.setWriteReadFailure(platform::ErrorCode::Ok);
    const auto retryResult = sensor.poll();

    ASSERT_TRUE(retryResult);
    ASSERT_EQ(i2c.writeReadCalls().size(), 1U);
    EXPECT_FALSE(sensor.isReady());
    for (std::size_t poll = 0U; poll < 12U; ++poll)
    {
        ASSERT_TRUE(sensor.poll());
    }
    EXPECT_TRUE(sensor.isReady());
}

TEST(Max30102Test, PollBeforeBeginReturnsInvalidState)
{
    MockI2c i2c;
    platform::drivers::max30102::Max30102 sensor(i2c);

    const auto result = sensor.poll();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::InvalidState);
    EXPECT_TRUE(i2c.writeReadCalls().empty());
}

TEST(Max30102Test, EmptyFifoDoesNotQueueSamples)
{
    MockI2c i2c;
    platform::drivers::max30102::Max30102 sensor(i2c);
    initializeSensor(i2c, sensor);

    ASSERT_TRUE(sensor.poll());

    EXPECT_FALSE(sensor.hasSample());
    const auto result = sensor.readSample();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::BufferEmpty);
}

TEST(Max30102Test, PollReadsAndDecodesOneFifoSample)
{
    constexpr std::uint8_t kFifoDataRegister{0x07U};
    MockI2c i2c;
    platform::drivers::max30102::Max30102 sensor(i2c);
    initializeSensor(i2c, sensor);
    i2c.setRegister(kFifoWritePointerRegister, 1U);
    i2c.setRegister(kFifoReadPointerRegister, 0U);
    i2c.setFifoData({0x01U, 0x23U, 0x45U, 0x02U, 0x34U, 0x56U});

    ASSERT_TRUE(sensor.poll());

    ASSERT_TRUE(sensor.hasSample());
    const auto sample = sensor.readSample();
    ASSERT_TRUE(sample);
    EXPECT_EQ(sample.value()->red, 0x12345U);
    EXPECT_EQ(sample.value()->infrared, 0x23456U);
    EXPECT_EQ(i2c.writeReadCalls().back().txData, (std::vector<std::uint8_t>{kFifoDataRegister}));
    EXPECT_EQ(i2c.writeReadCalls().back().rxSize, 6U);
}

TEST(Max30102Test, PollPreservesFifoSampleOrder)
{
    MockI2c i2c;
    platform::drivers::max30102::Max30102 sensor(i2c);
    initializeSensor(i2c, sensor);
    i2c.setRegister(kFifoWritePointerRegister, 3U);
    i2c.setRegister(kFifoReadPointerRegister, 0U);
    i2c.setFifoData({0U, 0U, 1U, 0U, 0U, 2U, 0U, 0U, 3U,
                     0U, 0U, 4U, 0U, 0U, 5U, 0U, 0U, 6U});

    ASSERT_TRUE(sensor.poll());

    for (std::uint32_t index = 0U; index < 3U; ++index)
    {
        const auto sample = sensor.readSample();
        ASSERT_TRUE(sample);
        EXPECT_EQ(sample.value()->red, (index * 2U) + 1U);
        EXPECT_EQ(sample.value()->infrared, (index * 2U) + 2U);
    }
    EXPECT_FALSE(sensor.hasSample());
}

TEST(Max30102Test, SampleDecoderMasksToEighteenBits)
{
    MockI2c i2c;
    platform::drivers::max30102::Max30102 sensor(i2c);
    initializeSensor(i2c, sensor);
    i2c.setRegister(kFifoWritePointerRegister, 1U);
    i2c.setRegister(kFifoReadPointerRegister, 0U);
    i2c.setFifoData({0xFFU, 0xFFU, 0xFFU, 0xFCU, 0x00U, 0x01U});

    ASSERT_TRUE(sensor.poll());

    const auto sample = sensor.readSample();
    ASSERT_TRUE(sample);
    EXPECT_EQ(sample.value()->red, 0x3FFFFU);
    EXPECT_EQ(sample.value()->infrared, 1U);
}

TEST(Max30102Test, PollDrainsNoMoreThanFourSamples)
{
    MockI2c i2c;
    platform::drivers::max30102::Max30102 sensor(i2c);
    initializeSensor(i2c, sensor);
    i2c.setRegister(kFifoWritePointerRegister, 5U);
    i2c.setRegister(kFifoReadPointerRegister, 0U);
    i2c.setFifoData({0U, 0U, 1U, 0U, 0U, 1U, 0U, 0U, 2U, 0U, 0U, 2U,
                     0U, 0U, 3U, 0U, 0U, 3U, 0U, 0U, 4U, 0U, 0U, 4U,
                     0U, 0U, 5U, 0U, 0U, 5U});

    ASSERT_TRUE(sensor.poll());
    EXPECT_EQ(i2c.writeReadCalls().back().rxSize, 24U);
    for (std::size_t index = 0U; index < 4U; ++index)
    {
        ASSERT_TRUE(sensor.readSample());
    }
    EXPECT_FALSE(sensor.hasSample());

    ASSERT_TRUE(sensor.poll());
    const auto fifthSample = sensor.readSample();
    ASSERT_TRUE(fifthSample);
    EXPECT_EQ(fifthSample.value()->red, 5U);
}

TEST(Max30102Test, HardwareOverflowIsReportedAndCounterCleared)
{
    constexpr std::uint8_t kFifoOverflowCounterRegister{0x05U};
    MockI2c i2c;
    platform::drivers::max30102::Max30102 sensor(i2c);
    initializeSensor(i2c, sensor);
    i2c.setRegister(kFifoWritePointerRegister, 0U);
    i2c.setRegister(kFifoReadPointerRegister, 0U);
    i2c.setRegister(kFifoOverflowCounterRegister, 3U);
    i2c.setFifoData({0U, 0U, 1U, 0U, 0U, 1U, 0U, 0U, 2U, 0U, 0U, 2U,
                     0U, 0U, 3U, 0U, 0U, 3U, 0U, 0U, 4U, 0U, 0U, 4U});

    ASSERT_TRUE(sensor.poll());

    EXPECT_EQ(sensor.hardwareOverflowCount(), 3U);
    EXPECT_EQ(i2c.writeCalls().back().data, (std::vector<std::uint8_t>{kFifoOverflowCounterRegister, 0U}));
    EXPECT_EQ(i2c.writeReadCalls().back().txData, (std::vector<std::uint8_t>{0x07U}));
    EXPECT_EQ(i2c.writeReadCalls().back().rxSize, 24U);
}

TEST(Max30102Test, FullSoftwareQueueDropsIncomingSamplesAndCountsThem)
{
    constexpr std::size_t kSamplesPerPoll{4U};
    constexpr std::size_t kPollsToFillQueue{
        platform::drivers::max30102::Max30102::kSampleQueueCapacity / kSamplesPerPoll};
    MockI2c i2c;
    platform::drivers::max30102::Max30102 sensor(i2c);
    initializeSensor(i2c, sensor);

    std::uint8_t readPointer{0U};
    for (std::size_t poll = 0U; poll < kPollsToFillQueue; ++poll)
    {
        const auto writePointer = static_cast<std::uint8_t>((readPointer + kSamplesPerPoll) & 0x1FU);
        i2c.setRegister(kFifoWritePointerRegister, writePointer);
        i2c.setRegister(kFifoReadPointerRegister, readPointer);

        std::vector<std::uint8_t> fifoData;
        for (std::uint32_t index = 0U; index < kSamplesPerPoll; ++index)
        {
            const auto value = static_cast<std::uint8_t>((poll * kSamplesPerPoll) + index + 1U);
            fifoData.insert(fifoData.end(), {0U, 0U, value, 0U, 0U, value});
        }
        i2c.setFifoData(fifoData);
        ASSERT_TRUE(sensor.poll());
        readPointer = writePointer;
    }
    ASSERT_TRUE(sensor.hasSample());
    EXPECT_EQ(sensor.softwareDroppedSamples(), 0U);

    i2c.setRegister(kFifoWritePointerRegister, 2U);
    i2c.setRegister(kFifoReadPointerRegister, readPointer);
    i2c.setFifoData({0U, 0U, 0xA1U, 0U, 0U, 0xA1U,
                     0U, 0U, 0xA2U, 0U, 0U, 0xA2U});
    ASSERT_TRUE(sensor.poll());

    EXPECT_EQ(sensor.softwareDroppedSamples(), 2U);
    EXPECT_TRUE(sensor.hasSample());
}

TEST(Max30102Test, ReadSampleBeforeReadyReturnsNotReady)
{
    MockI2c i2c;
    platform::drivers::max30102::Max30102 sensor(i2c);

    const auto result = sensor.readSample();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::NotReady);
}
