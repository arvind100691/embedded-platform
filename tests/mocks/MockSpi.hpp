#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "platform/hal/ISpi.hpp"

class MockSpi final : public platform::hal::ISpi
{
public:
    platform::Result<void> configure(const platform::hal::SpiConfig& config) override
    {
        config_ = config;
        configured_ = true;
        return platform::Result<void>::success();
    }

    platform::Result<void> transmit(const std::uint8_t* data, std::size_t size,
                                    std::uint32_t /*timeoutMs*/) override
    {
        if (!configured_)
        {
            return platform::Result<void>::failure(platform::ErrorCode::NotReady);
        }

        if (size == 0U)
        {
            return platform::Result<void>::success();
        }

        if (data == nullptr)
        {
            return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
        }

        transmittedData_.insert(transmittedData_.end(), data, data + size);
        return platform::Result<void>::success();
    }

    platform::Result<void> receive(std::uint8_t* data, std::size_t size,
                                   std::uint32_t /*timeoutMs*/) override
    {
        if (!configured_)
        {
            return platform::Result<void>::failure(platform::ErrorCode::NotReady);
        }

        if (size == 0U)
        {
            return platform::Result<void>::success();
        }

        if (data == nullptr)
        {
            return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
        }

        if (receivedData_.size() < size)
        {
            return platform::Result<void>::failure(platform::ErrorCode::BufferEmpty);
        }

        std::copy_n(receivedData_.begin(), size, data);
        receivedData_.erase(receivedData_.begin(), receivedData_.begin() + size);
        return platform::Result<void>::success();
    }

    platform::Result<void> transfer(const std::uint8_t* txData, std::uint8_t* rxData,
                                    std::size_t size, std::uint32_t timeoutMs) override
    {
        const auto transmitResult = transmit(txData, size, timeoutMs);
        if (!transmitResult)
        {
            return transmitResult;
        }

        return receive(rxData, size, timeoutMs);
    }

    void enqueueReceivedData(const std::vector<std::uint8_t>& data)
    {
        receivedData_.insert(receivedData_.end(), data.begin(), data.end());
    }

    [[nodiscard]] const platform::hal::SpiConfig& lastConfig() const
    {
        return config_;
    }

    [[nodiscard]] bool configured() const
    {
        return configured_;
    }

    [[nodiscard]] const std::vector<std::uint8_t>& transmittedData() const
    {
        return transmittedData_;
    }

private:
    platform::hal::SpiConfig config_{};
    std::vector<std::uint8_t> transmittedData_{};
    std::vector<std::uint8_t> receivedData_{};
    bool configured_{false};
};