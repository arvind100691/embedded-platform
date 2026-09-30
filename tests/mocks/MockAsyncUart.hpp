#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "platform/hal/IAsyncUart.hpp"

class MockAsyncUart final : public platform::hal::IAsyncUart
{
public:
    platform::Result<void> transmitAsync(const std::uint8_t* data, std::size_t size,
                                         platform::hal::UartTransferCallback callback,
                                         void* context) override
    {
        ++transmitCallCount_;

        if (data == nullptr || size == 0U || callback == nullptr)
        {
            return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
        }

        if (transmitActive_)
        {
            return platform::Result<void>::failure(platform::ErrorCode::Busy);
        }

        if (!transmitShouldStart_)
        {
            return platform::Result<void>::failure(transmitStartError_);
        }

        transmittedData_.assign(data, data + size);
        txCallback_ = callback;
        txContext_ = context;
        txSize_ = size;
        transmitActive_ = true;
        return platform::Result<void>::success();
    }

    platform::Result<void> receiveAsync(std::uint8_t* data, std::size_t size,
                                        platform::hal::UartTransferCallback callback,
                                        void* context) override
    {
        ++receiveCallCount_;

        if (data == nullptr || size == 0U || callback == nullptr)
        {
            return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
        }

        if (receiveActive_)
        {
            return platform::Result<void>::failure(platform::ErrorCode::Busy);
        }

        if (!receiveShouldStart_)
        {
            return platform::Result<void>::failure(receiveStartError_);
        }

        receiveBuffer_ = data;
        receiveSize_ = size;
        rxCallback_ = callback;
        rxContext_ = context;
        receiveActive_ = true;
        return platform::Result<void>::success();
    }

    platform::Result<void> cancelTransmit() override
    {
        ++cancelTransmitCallCount_;

        if (!transmitActive_)
        {
            return platform::Result<void>::failure(platform::ErrorCode::NotReady);
        }

        const auto callback = txCallback_;
        void* const context = txContext_;
        const std::size_t transferred = txTransferredBeforeCancel_;

        clearTransmit();

        callback(platform::hal::UartTransferStatus::Aborted, transferred, context);
        return platform::Result<void>::success();
    }

    platform::Result<void> cancelReceive() override
    {
        ++cancelReceiveCallCount_;

        if (!receiveActive_)
        {
            return platform::Result<void>::failure(platform::ErrorCode::NotReady);
        }

        const auto callback = rxCallback_;
        void* const context = rxContext_;
        const std::size_t transferred = rxTransferredBeforeCancel_;

        clearReceive();

        callback(platform::hal::UartTransferStatus::Aborted, transferred, context);
        return platform::Result<void>::success();
    }

    void completeTransmit(platform::hal::UartTransferStatus status, std::size_t transferredBytes)
    {
        if (!transmitActive_)
        {
            return;
        }

        const auto callback = txCallback_;
        void* const context = txContext_;
        clearTransmit();
        callback(status, transferredBytes, context);
    }

    void completeReceive(platform::hal::UartTransferStatus status, std::size_t transferredBytes)
    {
        if (!receiveActive_)
        {
            return;
        }

        const auto callback = rxCallback_;
        void* const context = rxContext_;
        clearReceive();
        callback(status, transferredBytes, context);
    }

    bool completeReceiveByte(std::uint8_t value)
    {
        if (!receiveActive_ || receiveBuffer_ == nullptr || receiveSize_ == 0U)
        {
            return false;
        }

        receiveBuffer_[0] = value;
        completeReceive(platform::hal::UartTransferStatus::Completed, 1U);
        return true;
    }

    void setTransmitStartFailure(platform::ErrorCode error)
    {
        transmitShouldStart_ = false;
        transmitStartError_ = error;
    }

    void setReceiveStartFailure(platform::ErrorCode error)
    {
        receiveShouldStart_ = false;
        receiveStartError_ = error;
    }

    void setTransmitTransferredBeforeCancel(std::size_t transferredBytes)
    {
        txTransferredBeforeCancel_ = transferredBytes;
    }

    void setReceiveTransferredBeforeCancel(std::size_t transferredBytes)
    {
        rxTransferredBeforeCancel_ = transferredBytes;
    }

    [[nodiscard]] bool transmitActive() const
    {
        return transmitActive_;
    }

    [[nodiscard]] bool receiveActive() const
    {
        return receiveActive_;
    }

    [[nodiscard]] const std::vector<std::uint8_t>& transmittedData() const
    {
        return transmittedData_;
    }

    [[nodiscard]] std::size_t transmitCallCount() const
    {
        return transmitCallCount_;
    }

    [[nodiscard]] std::size_t receiveCallCount() const
    {
        return receiveCallCount_;
    }

    [[nodiscard]] std::size_t cancelTransmitCallCount() const
    {
        return cancelTransmitCallCount_;
    }

    [[nodiscard]] std::size_t cancelReceiveCallCount() const
    {
        return cancelReceiveCallCount_;
    }

private:
    void clearTransmit()
    {
        transmitActive_ = false;
        txCallback_ = nullptr;
        txContext_ = nullptr;
        txSize_ = 0U;
    }

    void clearReceive()
    {
        receiveActive_ = false;
        receiveBuffer_ = nullptr;
        receiveSize_ = 0U;
        rxCallback_ = nullptr;
        rxContext_ = nullptr;
    }

private:
    std::vector<std::uint8_t> transmittedData_{};
    std::uint8_t* receiveBuffer_{nullptr};

    platform::hal::UartTransferCallback txCallback_{nullptr};
    void* txContext_{nullptr};
    platform::hal::UartTransferCallback rxCallback_{nullptr};
    void* rxContext_{nullptr};

    std::size_t txSize_{0U};
    std::size_t receiveSize_{0U};
    std::size_t txTransferredBeforeCancel_{0U};
    std::size_t rxTransferredBeforeCancel_{0U};

    platform::ErrorCode transmitStartError_{platform::ErrorCode::CommunicationError};
    platform::ErrorCode receiveStartError_{platform::ErrorCode::CommunicationError};

    bool transmitShouldStart_{true};
    bool receiveShouldStart_{true};
    bool transmitActive_{false};
    bool receiveActive_{false};

    std::size_t transmitCallCount_{0U};
    std::size_t receiveCallCount_{0U};
    std::size_t cancelTransmitCallCount_{0U};
    std::size_t cancelReceiveCallCount_{0U};
};
