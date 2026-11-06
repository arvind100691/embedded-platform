#include "platform/drivers/max30102/Max30102.hpp"

#include <algorithm>
#include <array>

namespace platform::drivers::max30102
{
namespace
{

constexpr std::uint8_t kPartIdRegister{0xFFU};
constexpr std::uint8_t kExpectedPartId{0x15U};
constexpr std::uint8_t kFifoDataRegister{0x07U};
constexpr std::uint8_t kFifoWritePointerRegister{0x04U};
constexpr std::uint8_t kFifoOverflowCounterRegister{0x05U};
constexpr std::uint8_t kFifoReadPointerRegister{0x06U};
constexpr std::uint8_t kFifoConfigurationRegister{0x08U};
constexpr std::uint8_t kModeConfigurationRegister{0x09U};
constexpr std::uint8_t kSpo2ConfigurationRegister{0x0AU};
constexpr std::uint8_t kRedLedCurrentRegister{0x0CU};
constexpr std::uint8_t kInfraredLedCurrentRegister{0x0DU};
constexpr std::uint8_t kResetBit{0x40U};
constexpr std::uint8_t kFifoRolloverEnabled{0x10U};
constexpr std::uint8_t kRedAndInfraredMode{0x03U};
constexpr std::uint8_t kSpo2Configuration100Hz411Us{0x07U};
constexpr std::uint8_t kFifoPointerMask{0x1FU};
constexpr std::size_t kBytesPerSample{6U};
constexpr std::uint32_t kTransactionTimeoutMs{100U};

} // namespace

Max30102::Max30102(platform::hal::II2c& i2c, Max30102Config config)
    : i2c_(i2c)
    , config_(config)
{
}

platform::Result<void> Max30102::begin()
{
    queueHead_ = 0U;
    queueTail_ = 0U;
    queuedSamples_ = 0U;
    hardwareOverflowCount_ = 0U;
    softwareDroppedSamples_ = 0U;
    state_ = State::CheckingPartId;
    return platform::Result<void>::success();
}

platform::Result<void> Max30102::poll()
{
    if (state_ == State::NotStarted || state_ == State::Failed)
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidState);
    }

    if (state_ == State::Ready)
    {
        return serviceFifo();
    }

    switch (state_)
    {
    case State::CheckingPartId:
    {
        const auto partId = readRegister(kPartIdRegister);
        if (!partId)
        {
            return platform::Result<void>::failure(partId.error());
        }

        if (*partId.value() != kExpectedPartId)
        {
            state_ = State::Failed;
            return platform::Result<void>::failure(platform::ErrorCode::NotFound);
        }

        state_ = State::RequestingReset;
        return platform::Result<void>::success();
    }
    case State::RequestingReset:
        return advanceAfterWrite(kModeConfigurationRegister, kResetBit, State::WaitingForReset);
    case State::WaitingForReset:
    {
        const auto mode = readRegister(kModeConfigurationRegister);
        if (!mode)
        {
            return platform::Result<void>::failure(mode.error());
        }
        if ((*mode.value() & kResetBit) == 0U)
        {
            state_ = State::ConfiguringFifo;
        }
        return platform::Result<void>::success();
    }
    case State::ConfiguringFifo:
        return advanceAfterWrite(kFifoConfigurationRegister, kFifoRolloverEnabled,
                                State::ConfiguringMode);
    case State::ConfiguringMode:
        return advanceAfterWrite(kModeConfigurationRegister, kRedAndInfraredMode,
                                State::ConfiguringSpo2);
    case State::ConfiguringSpo2:
        return advanceAfterWrite(kSpo2ConfigurationRegister, kSpo2Configuration100Hz411Us,
                                State::ConfiguringRedLed);
    case State::ConfiguringRedLed:
        return advanceAfterWrite(kRedLedCurrentRegister, config_.redLedCurrent,
                                State::ConfiguringInfraredLed);
    case State::ConfiguringInfraredLed:
        return advanceAfterWrite(kInfraredLedCurrentRegister, config_.infraredLedCurrent,
                                State::ClearingWritePointer);
    case State::ClearingWritePointer:
        return advanceAfterWrite(kFifoWritePointerRegister, 0U, State::ClearingOverflowCounter);
    case State::ClearingOverflowCounter:
        return advanceAfterWrite(kFifoOverflowCounterRegister, 0U, State::ClearingReadPointer);
    case State::ClearingReadPointer:
        return advanceAfterWrite(kFifoReadPointerRegister, 0U, State::Ready);
    case State::NotStarted:
    case State::Ready:
    case State::Failed:
        break;
    }

    state_ = State::Failed;
    return platform::Result<void>::failure(platform::ErrorCode::InvalidState);
}

bool Max30102::isReady() const noexcept
{
    return state_ == State::Ready;
}

bool Max30102::hasSample() const noexcept
{
    return queuedSamples_ > 0U;
}

std::uint8_t Max30102::hardwareOverflowCount() const noexcept
{
    return hardwareOverflowCount_;
}

std::uint32_t Max30102::softwareDroppedSamples() const noexcept
{
    return softwareDroppedSamples_;
}

platform::Result<Max30102Sample> Max30102::readSample()
{
    if (!isReady())
    {
        return platform::Result<Max30102Sample>::failure(platform::ErrorCode::NotReady);
    }
    if (queuedSamples_ == 0U)
    {
        return platform::Result<Max30102Sample>::failure(platform::ErrorCode::BufferEmpty);
    }

    const auto sample = sampleQueue_[queueTail_];
    queueTail_ = (queueTail_ + 1U) % sampleQueue_.size();
    --queuedSamples_;
    return platform::Result<Max30102Sample>::success(sample);
}

platform::Result<std::uint8_t> Max30102::readRegister(std::uint8_t reg)
{
    const std::array<std::uint8_t, 1U> txData{reg};
    std::uint8_t value{0U};

    const auto result =
        i2c_.writeRead(kI2cAddress, txData.data(), txData.size(), &value, 1U, kTransactionTimeoutMs);
    if (!result)
    {
        return platform::Result<std::uint8_t>::failure(result.error());
    }

    return platform::Result<std::uint8_t>::success(value);
}

platform::Result<void> Max30102::serviceFifo()
{
    const std::array<std::uint8_t, 1U> txData{kFifoWritePointerRegister};
    std::array<std::uint8_t, 3U> fifoStatus{};
    const auto statusResult =
        i2c_.writeRead(kI2cAddress, txData.data(), txData.size(), fifoStatus.data(),
                       fifoStatus.size(), kTransactionTimeoutMs);
    if (!statusResult)
    {
        return statusResult;
    }

    const auto writePointer = static_cast<std::size_t>(fifoStatus[0] & kFifoPointerMask);
    const auto overflowCount = static_cast<std::uint8_t>(fifoStatus[1] & kFifoPointerMask);
    const auto readPointer = static_cast<std::size_t>(fifoStatus[2] & kFifoPointerMask);
    hardwareOverflowCount_ = overflowCount;

    const auto pendingSamples = overflowCount > 0U
                                   ? kFifoPointerMask + 1U
                                   : (writePointer - readPointer) & kFifoPointerMask;
    if (overflowCount > 0U)
    {
        const auto clearResult = writeRegister(kFifoOverflowCounterRegister, 0U);
        if (!clearResult)
        {
            return clearResult;
        }
    }

    const auto samplesToRead = std::min(pendingSamples, kMaxSamplesPerPoll);
    if (samplesToRead == 0U)
    {
        return platform::Result<void>::success();
    }

    return readFifoSamples(samplesToRead);
}

platform::Result<void> Max30102::readFifoSamples(std::size_t sampleCount)
{
    std::array<std::uint8_t, kMaxSamplesPerPoll * kBytesPerSample> bytes{};
    const std::array<std::uint8_t, 1U> txData{kFifoDataRegister};
    const auto byteCount = sampleCount * kBytesPerSample;
    const auto result =
        i2c_.writeRead(kI2cAddress, txData.data(), txData.size(), bytes.data(), byteCount,
                       kTransactionTimeoutMs);
    if (!result)
    {
        return result;
    }

    for (std::size_t offset = 0U; offset < byteCount; offset += kBytesPerSample)
    {
        enqueueSample(decodeSample(bytes.data() + offset));
    }
    return platform::Result<void>::success();
}

platform::Result<void> Max30102::writeRegister(std::uint8_t reg, std::uint8_t value)
{
    const std::array<std::uint8_t, 2U> txData{reg, value};
    return i2c_.write(kI2cAddress, txData.data(), txData.size(), kTransactionTimeoutMs);
}

platform::Result<void> Max30102::advanceAfterWrite(std::uint8_t reg, std::uint8_t value,
                                                   State nextState)
{
    const auto result = writeRegister(reg, value);
    if (!result)
    {
        return result;
    }

    state_ = nextState;
    return platform::Result<void>::success();
}

void Max30102::enqueueSample(Max30102Sample sample)
{
    if (queuedSamples_ == sampleQueue_.size())
    {
        if (softwareDroppedSamples_ < UINT32_MAX)
        {
            ++softwareDroppedSamples_;
        }
        return;
    }

    sampleQueue_[queueHead_] = sample;
    queueHead_ = (queueHead_ + 1U) % sampleQueue_.size();
    ++queuedSamples_;
}

Max30102Sample Max30102::decodeSample(const std::uint8_t* bytes)
{
    constexpr std::uint32_t kAdcValueMask{0x3FFFFU};
    const auto red =
        (static_cast<std::uint32_t>(bytes[0]) << 16U) |
        (static_cast<std::uint32_t>(bytes[1]) << 8U) | static_cast<std::uint32_t>(bytes[2]);
    const auto infrared =
        (static_cast<std::uint32_t>(bytes[3]) << 16U) |
        (static_cast<std::uint32_t>(bytes[4]) << 8U) | static_cast<std::uint32_t>(bytes[5]);
    return Max30102Sample{red & kAdcValueMask, infrared & kAdcValueMask};
}

} // namespace platform::drivers::max30102
