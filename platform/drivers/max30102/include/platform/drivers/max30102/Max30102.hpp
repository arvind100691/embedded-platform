#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "platform/hal/II2c.hpp"

namespace platform::drivers::max30102
{

struct Max30102Config
{
    std::uint8_t redLedCurrent{0x10U};
    std::uint8_t infraredLedCurrent{0x10U};
};

struct Max30102Sample
{
    std::uint32_t red;
    std::uint32_t infrared;
};

class Max30102 final
{
public:
    static constexpr platform::hal::I2cAddress kI2cAddress{0x57U};
    static constexpr std::size_t kSampleQueueCapacity{32U};
    static constexpr std::size_t kMaxSamplesPerPoll{4U};

    explicit Max30102(platform::hal::II2c& i2c, Max30102Config config = {});

    platform::Result<void> begin();
    platform::Result<void> poll();

    [[nodiscard]] bool isReady() const noexcept;
    [[nodiscard]] bool hasSample() const noexcept;
    [[nodiscard]] std::uint8_t hardwareOverflowCount() const noexcept;
    [[nodiscard]] std::uint32_t softwareDroppedSamples() const noexcept;
    platform::Result<Max30102Sample> readSample();

private:
    enum class State
    {
        NotStarted,
        CheckingPartId,
        RequestingReset,
        WaitingForReset,
        ConfiguringFifo,
        ConfiguringMode,
        ConfiguringSpo2,
        ConfiguringRedLed,
        ConfiguringInfraredLed,
        ClearingWritePointer,
        ClearingOverflowCounter,
        ClearingReadPointer,
        Ready,
        Failed
    };

    platform::Result<std::uint8_t> readRegister(std::uint8_t reg);
    platform::Result<void> serviceFifo();
    platform::Result<void> readFifoSamples(std::size_t sampleCount);
    platform::Result<void> writeRegister(std::uint8_t reg, std::uint8_t value);
    platform::Result<void> advanceAfterWrite(std::uint8_t reg, std::uint8_t value,
                                             State nextState);
    void enqueueSample(Max30102Sample sample);
    static Max30102Sample decodeSample(const std::uint8_t* bytes);

    platform::hal::II2c& i2c_;
    Max30102Config config_;
    State state_{State::NotStarted};
    std::array<Max30102Sample, kSampleQueueCapacity> sampleQueue_{};
    std::size_t queueHead_{0U};
    std::size_t queueTail_{0U};
    std::size_t queuedSamples_{0U};
    std::uint8_t hardwareOverflowCount_{0U};
    std::uint32_t softwareDroppedSamples_{0U};
};

} // namespace platform::drivers::max30102
