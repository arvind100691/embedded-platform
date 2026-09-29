#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "platform/hal/IUart.hpp"

class MockUart final : public platform::hal::IUart
{
public:
    platform::Result<void> configure(const platform::hal::UartConfig& config) override
    {
        config_ = config;
        ++configureCallCount_;

        if (!configureShouldSucceed_)
        {
            return platform::Result<void>::failure(configureError_);
        }

        configured_ = true;
        return platform::Result<void>::success();
    }

    platform::Result<void> transmit(const std::uint8_t* data, std::size_t size,
                                    std::uint32_t /*timeoutMs*/) override
    {
        ++transmitCallCount_;

        if (size == 0U)
        {
            return platform::Result<void>::success();
        }

        if (data == nullptr)
        {
            return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
        }

        if (!transmitShouldSucceed_)
        {
            return platform::Result<void>::failure(transmitError_);
        }

        transmittedData_.insert(transmittedData_.end(), data, data + size);
        return platform::Result<void>::success();
    }

    platform::Result<std::size_t> receive(std::uint8_t* data, std::size_t size,
                                          std::uint32_t /*timeoutMs*/) override
    {
        ++receiveCallCount_;

        if (size == 0U)
        {
            return platform::Result<std::size_t>::success(0U);
        }

        if (data == nullptr)
        {
            return platform::Result<std::size_t>::failure(platform::ErrorCode::InvalidArgument);
        }

        if (!receiveShouldSucceed_)
        {
            return platform::Result<std::size_t>::failure(receiveError_);
        }

        if (receivedData_.empty())
        {
            return platform::Result<std::size_t>::failure(platform::ErrorCode::BufferEmpty);
        }

        const std::size_t count = std::min(size, receivedData_.size());

        std::copy_n(receivedData_.begin(), count, data);
        receivedData_.erase(receivedData_.begin(), receivedData_.begin() + count);

        return platform::Result<std::size_t>::success(count);
    }

    void enqueueReceivedData(const std::uint8_t* data, std::size_t size)
    {
        if (data == nullptr || size == 0U)
        {
            return;
        }

        receivedData_.insert(receivedData_.end(), data, data + size);
    }

    void enqueueReceivedData(const std::vector<std::uint8_t>& data)
    {
        receivedData_.insert(receivedData_.end(), data.begin(), data.end());
    }

    void clearTransmittedData()
    {
        transmittedData_.clear();
    }

    void setConfigureFailure(platform::ErrorCode error)
    {
        configureShouldSucceed_ = false;
        configureError_ = error;
    }

    void setConfigureSuccess()
    {
        configureShouldSucceed_ = true;
    }

    void setTransmitFailure(platform::ErrorCode error)
    {
        transmitShouldSucceed_ = false;
        transmitError_ = error;
    }

    void setTransmitSuccess()
    {
        transmitShouldSucceed_ = true;
    }

    void setReceiveFailure(platform::ErrorCode error)
    {
        receiveShouldSucceed_ = false;
        receiveError_ = error;
    }

    void setReceiveSuccess()
    {
        receiveShouldSucceed_ = true;
    }

    [[nodiscard]] const platform::hal::UartConfig& lastConfig() const
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

    [[nodiscard]] std::size_t configureCallCount() const
    {
        return configureCallCount_;
    }

    [[nodiscard]] std::size_t transmitCallCount() const
    {
        return transmitCallCount_;
    }

    [[nodiscard]] std::size_t receiveCallCount() const
    {
        return receiveCallCount_;
    }

private:
    platform::hal::UartConfig config_{};

    std::vector<std::uint8_t> transmittedData_{};
    std::vector<std::uint8_t> receivedData_{};

    platform::ErrorCode configureError_{platform::ErrorCode::HardwareFault};
    platform::ErrorCode transmitError_{platform::ErrorCode::CommunicationError};
    platform::ErrorCode receiveError_{platform::ErrorCode::CommunicationError};

    bool configureShouldSucceed_{true};
    bool transmitShouldSucceed_{true};
    bool receiveShouldSucceed_{true};
    bool configured_{false};

    std::size_t configureCallCount_{0U};
    std::size_t transmitCallCount_{0U};
    std::size_t receiveCallCount_{0U};
};
