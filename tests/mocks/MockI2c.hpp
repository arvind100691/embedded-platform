#pragma once

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "platform/hal/II2c.hpp"

class MockI2c final : public platform::hal::II2c
{
public:
    struct WriteCall
    {
        platform::hal::I2cAddress address;
        std::vector<std::uint8_t> data;
        std::uint32_t timeoutMs;
    };

    struct WriteReadCall
    {
        platform::hal::I2cAddress address;
        std::vector<std::uint8_t> txData;
        std::size_t rxSize;
        std::uint32_t timeoutMs;
    };

    platform::Result<void> configure(const platform::hal::I2cConfig& config) override
    {
        config_ = config;
        return platform::Result<void>::success();
    }

    platform::Result<void> write(platform::hal::I2cAddress address, const std::uint8_t* data,
                                 std::size_t size, std::uint32_t timeoutMs) override
    {
        if (writeFailure_ != platform::ErrorCode::Ok)
        {
            return platform::Result<void>::failure(writeFailure_);
        }
        if (data == nullptr || size != 2U)
        {
            return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
        }

        writeCalls_.push_back(
            WriteCall{address, std::vector<std::uint8_t>(data, data + size), timeoutMs});

        const auto reg = data[0];
        registers_[reg] = data[1];
        if (reg == kModeConfigurationRegister && (data[1] & kResetBit) != 0U)
        {
            registers_[reg] = kResetBit;
        }
        return platform::Result<void>::success();
    }

    platform::Result<void> read(platform::hal::I2cAddress, std::uint8_t*, std::size_t,
                                std::uint32_t) override
    {
        return platform::Result<void>::failure(platform::ErrorCode::Unsupported);
    }

    platform::Result<void> writeRead(platform::hal::I2cAddress address,
                                     const std::uint8_t* txData, std::size_t txSize,
                                     std::uint8_t* rxData, std::size_t rxSize,
                                     std::uint32_t timeoutMs) override
    {
        if (writeReadFailure_ != platform::ErrorCode::Ok)
        {
            return platform::Result<void>::failure(writeReadFailure_);
        }
        if (txData == nullptr || rxData == nullptr || txSize != 1U || rxSize == 0U)
        {
            return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
        }

        writeReadCalls_.push_back(
            WriteReadCall{address, std::vector<std::uint8_t>(txData, txData + txSize), rxSize,
                          timeoutMs});
        const auto reg = txData[0];
        if (reg == kFifoDataRegister)
        {
            if (fifoData_.size() < rxSize || rxSize % kBytesPerSample != 0U)
            {
                return platform::Result<void>::failure(platform::ErrorCode::BufferEmpty);
            }

            std::copy_n(fifoData_.begin(), rxSize, rxData);
            fifoData_.erase(fifoData_.begin(), fifoData_.begin() + static_cast<std::ptrdiff_t>(rxSize));
            registers_[kFifoReadPointerRegister] = static_cast<std::uint8_t>(
                (registers_[kFifoReadPointerRegister] + (rxSize / kBytesPerSample)) & kFifoPointerMask);
        }
        else
        {
            if (static_cast<std::size_t>(reg) + rxSize > registers_.size())
            {
                return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
            }
            std::copy_n(registers_.begin() + reg, rxSize, rxData);
            if (reg == kModeConfigurationRegister && (registers_[reg] & kResetBit) != 0U)
            {
                if (resetPendingReads_ > 0U)
                {
                    --resetPendingReads_;
                }
                else
                {
                    registers_[reg] = 0U;
                    *rxData = 0U;
                }
            }
        }
        return platform::Result<void>::success();
    }

    void setRegister(std::uint8_t reg, std::uint8_t value)
    {
        registers_[reg] = value;
    }

    void setResetPendingReads(std::size_t reads)
    {
        resetPendingReads_ = reads;
    }

    void setFifoData(const std::vector<std::uint8_t>& data)
    {
        fifoData_ = data;
    }

    void setWriteFailure(platform::ErrorCode error)
    {
        writeFailure_ = error;
    }

    void setWriteReadFailure(platform::ErrorCode error)
    {
        writeReadFailure_ = error;
    }

    [[nodiscard]] const std::vector<WriteCall>& writeCalls() const
    {
        return writeCalls_;
    }

    [[nodiscard]] const std::vector<WriteReadCall>& writeReadCalls() const
    {
        return writeReadCalls_;
    }

private:
    static constexpr std::uint8_t kModeConfigurationRegister{0x09U};
    static constexpr std::uint8_t kResetBit{0x40U};
    static constexpr std::uint8_t kFifoDataRegister{0x07U};
    static constexpr std::uint8_t kFifoReadPointerRegister{0x06U};
    static constexpr std::uint8_t kFifoPointerMask{0x1FU};
    static constexpr std::size_t kBytesPerSample{6U};

    platform::hal::I2cConfig config_{};
    std::array<std::uint8_t, 256U> registers_{};
    std::vector<std::uint8_t> fifoData_{};
    std::vector<WriteCall> writeCalls_{};
    std::vector<WriteReadCall> writeReadCalls_{};
    std::size_t resetPendingReads_{0U};
    platform::ErrorCode writeFailure_{platform::ErrorCode::Ok};
    platform::ErrorCode writeReadFailure_{platform::ErrorCode::Ok};
};
