#pragma once

#include <array>
#include <atomic>
#include <cstddef>

#include "platform/common/NonCopyable.hpp"
#include "platform/common/Result.hpp"
#include "platform/common/SpscByteRingBuffer.hpp"
#include "platform/hal/IAsyncUart.hpp"

namespace platform::middleware::communication
{

template <std::size_t RxCapacity, std::size_t TxCapacity, std::size_t TxChunkSize = 16U>
class BufferedAsyncUart final : public platform::NonCopyable
{
    static_assert(RxCapacity > 1U, "RxCapacity must be greater than one");
    static_assert(TxCapacity > 1U, "TxCapacity must be greater than one");
    static_assert(TxChunkSize > 0U, "TxChunkSize must be greater than zero");
    static_assert(TxChunkSize <= TxCapacity, "TxChunkSize must not exceed TxCapacity");
    static_assert(std::atomic<bool>::is_always_lock_free,
                  "BufferedAsyncUart requires lock-free bool atomics");
    static_assert(std::atomic<std::size_t>::is_always_lock_free,
                  "BufferedAsyncUart requires lock-free size_t atomics");

public:
    explicit BufferedAsyncUart(platform::hal::IAsyncUart& uart) noexcept
        : uart_(uart)
    {
    }

    platform::Result<void> start() noexcept
    {
        if (running_.load(std::memory_order_acquire))
        {
            return platform::Result<void>::failure(platform::ErrorCode::InvalidState);
        }

        if (rxActive_.load(std::memory_order_acquire) || txActive_.load(std::memory_order_acquire))
        {
            return platform::Result<void>::failure(platform::ErrorCode::Busy);
        }

        rxBuffer_.clear();
        txBuffer_.clear();
        rxOverflowed_.store(false, std::memory_order_relaxed);
        rxError_.store(false, std::memory_order_relaxed);
        txError_.store(false, std::memory_order_relaxed);
        rxNeedsRearm_.store(false, std::memory_order_relaxed);
        txInFlightSize_.store(0U, std::memory_order_relaxed);

        running_.store(true, std::memory_order_release);

        const auto result = armReceive();
        if (!result)
        {
            running_.store(false, std::memory_order_release);
            return result;
        }

        return platform::Result<void>::success();
    }

    platform::Result<void> stop() noexcept
    {
        if (!running_.exchange(false, std::memory_order_acq_rel))
        {
            return platform::Result<void>::failure(platform::ErrorCode::NotReady);
        }

        platform::ErrorCode cancelError = platform::ErrorCode::Ok;

        if (rxActive_.load(std::memory_order_acquire))
        {
            const auto result = uart_.cancelReceive();
            if (!result && result.error() == platform::ErrorCode::NotReady)
            {
                rxActive_.store(false, std::memory_order_release);
            }
            else if (!result)
            {
                cancelError = result.error();
            }
        }

        if (txActive_.load(std::memory_order_acquire))
        {
            const auto result = uart_.cancelTransmit();
            if (!result && result.error() == platform::ErrorCode::NotReady)
            {
                txActive_.store(false, std::memory_order_release);
            }
            else if (!result && cancelError == platform::ErrorCode::Ok)
            {
                cancelError = result.error();
            }
        }

        rxNeedsRearm_.store(false, std::memory_order_release);

        if (!rxActive_.load(std::memory_order_acquire) &&
            !txActive_.load(std::memory_order_acquire))
        {
            rxBuffer_.clear();
            txBuffer_.clear();
            txInFlightSize_.store(0U, std::memory_order_relaxed);
        }

        if (cancelError != platform::ErrorCode::Ok)
        {
            return platform::Result<void>::failure(cancelError);
        }

        return platform::Result<void>::success();
    }

    platform::Result<std::size_t> write(const platform::Byte* data, std::size_t size) noexcept
    {
        if (!running_.load(std::memory_order_acquire))
        {
            return platform::Result<std::size_t>::failure(platform::ErrorCode::NotReady);
        }

        if (data == nullptr || size == 0U)
        {
            return platform::Result<std::size_t>::failure(platform::ErrorCode::InvalidArgument);
        }

        if (txError_.load(std::memory_order_acquire))
        {
            return platform::Result<std::size_t>::failure(platform::ErrorCode::CommunicationError);
        }

        if (size > txBuffer_.freeSpace())
        {
            return platform::Result<std::size_t>::failure(platform::ErrorCode::BufferFull);
        }

        const std::size_t written = txBuffer_.write(data, size);
        const auto result = serviceTransmit();
        if (!result && result.error() != platform::ErrorCode::Busy)
        {
            return platform::Result<std::size_t>::failure(result.error());
        }

        return platform::Result<std::size_t>::success(written);
    }

    platform::Result<std::size_t> read(platform::Byte* data, std::size_t size) noexcept
    {
        if (!running_.load(std::memory_order_acquire))
        {
            return platform::Result<std::size_t>::failure(platform::ErrorCode::NotReady);
        }

        if (data == nullptr || size == 0U)
        {
            return platform::Result<std::size_t>::failure(platform::ErrorCode::InvalidArgument);
        }

        if (rxBuffer_.empty())
        {
            return platform::Result<std::size_t>::failure(platform::ErrorCode::BufferEmpty);
        }

        return platform::Result<std::size_t>::success(rxBuffer_.read(data, size));
    }

    platform::Result<void> poll() noexcept
    {
        if (!running_.load(std::memory_order_acquire))
        {
            return platform::Result<void>::failure(platform::ErrorCode::NotReady);
        }

        if (rxNeedsRearm_.exchange(false, std::memory_order_acq_rel))
        {
            const auto result = armReceive();
            if (!result)
            {
                if (result.error() == platform::ErrorCode::Busy)
                {
                    rxNeedsRearm_.store(true, std::memory_order_release);
                }
                else
                {
                    rxError_.store(true, std::memory_order_release);
                }
            }
        }

        if (rxError_.load(std::memory_order_acquire) || txError_.load(std::memory_order_acquire))
        {
            return platform::Result<void>::failure(platform::ErrorCode::CommunicationError);
        }

        const auto result = serviceTransmit();
        if (!result && result.error() == platform::ErrorCode::Busy)
        {
            return platform::Result<void>::success();
        }

        return result;
    }

    [[nodiscard]] std::size_t available() const noexcept
    {
        return rxBuffer_.size();
    }

    [[nodiscard]] std::size_t freeTxSpace() const noexcept
    {
        return txBuffer_.freeSpace();
    }

    [[nodiscard]] bool rxOverflowed() const noexcept
    {
        return rxOverflowed_.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool rxError() const noexcept
    {
        return rxError_.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool txError() const noexcept
    {
        return txError_.load(std::memory_order_acquire);
    }

    void clearRxOverflow() noexcept
    {
        rxOverflowed_.store(false, std::memory_order_release);
    }

    void clearErrors() noexcept
    {
        const bool hadRxError = rxError_.exchange(false, std::memory_order_acq_rel);
        txError_.store(false, std::memory_order_release);
        rxOverflowed_.store(false, std::memory_order_release);

        if (hadRxError && running_.load(std::memory_order_acquire))
        {
            rxNeedsRearm_.store(true, std::memory_order_release);
        }
    }

    [[nodiscard]] bool running() const noexcept
    {
        return running_.load(std::memory_order_acquire);
    }

private:
    platform::Result<void> armReceive() noexcept
    {
        if (!running_.load(std::memory_order_acquire) || rxActive_.load(std::memory_order_acquire))
        {
            return platform::Result<void>::success();
        }

        rxActive_.store(true, std::memory_order_release);
        const auto result =
            uart_.receiveAsync(&rxStagingByte_, 1U, &BufferedAsyncUart::receiveCallback, this);

        if (!result)
        {
            rxActive_.store(false, std::memory_order_release);
        }

        return result;
    }

    platform::Result<void> serviceTransmit() noexcept
    {
        if (!running_.load(std::memory_order_acquire))
        {
            return platform::Result<void>::failure(platform::ErrorCode::NotReady);
        }

        if (txError_.load(std::memory_order_acquire))
        {
            return platform::Result<void>::failure(platform::ErrorCode::CommunicationError);
        }

        if (txActive_.load(std::memory_order_acquire) || txBuffer_.empty())
        {
            return platform::Result<void>::success();
        }

        const std::size_t count = (txBuffer_.size() < TxChunkSize) ? txBuffer_.size() : TxChunkSize;

        if (txBuffer_.peek(txChunk_.data(), count) != count)
        {
            return platform::Result<void>::failure(platform::ErrorCode::InternalError);
        }

        txInFlightSize_.store(count, std::memory_order_relaxed);
        txActive_.store(true, std::memory_order_release);
        const auto result =
            uart_.transmitAsync(txChunk_.data(), count, &BufferedAsyncUart::transmitCallback, this);

        if (!result)
        {
            txInFlightSize_.store(0U, std::memory_order_relaxed);
            txActive_.store(false, std::memory_order_release);
            if (result.error() != platform::ErrorCode::Busy)
            {
                txError_.store(true, std::memory_order_release);
                return platform::Result<void>::failure(platform::ErrorCode::CommunicationError);
            }
        }

        return result;
    }

    static void transmitCallback(platform::hal::UartTransferStatus status,
                                 std::size_t transferredBytes, void* context)
    {
        auto* self = static_cast<BufferedAsyncUart*>(context);
        if (self != nullptr)
        {
            self->handleTransmitComplete(status, transferredBytes);
        }
    }

    static void receiveCallback(platform::hal::UartTransferStatus status,
                                std::size_t transferredBytes, void* context)
    {
        auto* self = static_cast<BufferedAsyncUart*>(context);
        if (self != nullptr)
        {
            self->handleReceiveComplete(status, transferredBytes);
        }
    }

    void handleTransmitComplete(platform::hal::UartTransferStatus status,
                                std::size_t transferredBytes) noexcept
    {
        const std::size_t inFlightSize = txInFlightSize_.load(std::memory_order_relaxed);

        if (!running_.load(std::memory_order_acquire))
        {
            txInFlightSize_.store(0U, std::memory_order_relaxed);
            txActive_.store(false, std::memory_order_release);
            return;
        }

        if (status != platform::hal::UartTransferStatus::Completed ||
            transferredBytes != inFlightSize || !txBuffer_.discard(inFlightSize))
        {
            txError_.store(true, std::memory_order_release);
        }

        txInFlightSize_.store(0U, std::memory_order_relaxed);
        txActive_.store(false, std::memory_order_release);
    }

    void handleReceiveComplete(platform::hal::UartTransferStatus status,
                               std::size_t transferredBytes) noexcept
    {
        rxActive_.store(false, std::memory_order_release);

        if (!running_.load(std::memory_order_acquire))
        {
            return;
        }

        if (status != platform::hal::UartTransferStatus::Completed || transferredBytes != 1U)
        {
            rxError_.store(true, std::memory_order_release);
            return;
        }

        if (!rxBuffer_.push(rxStagingByte_))
        {
            rxOverflowed_.store(true, std::memory_order_release);
        }

        rxNeedsRearm_.store(true, std::memory_order_release);
    }

    platform::hal::IAsyncUart& uart_;
    platform::SpscByteRingBuffer<RxCapacity> rxBuffer_{};
    platform::SpscByteRingBuffer<TxCapacity> txBuffer_{};
    std::array<platform::Byte, TxChunkSize> txChunk_{};
    platform::Byte rxStagingByte_{0U};

    std::atomic<bool> running_{false};
    std::atomic<bool> rxActive_{false};
    std::atomic<bool> txActive_{false};
    std::atomic<bool> rxOverflowed_{false};
    std::atomic<bool> rxError_{false};
    std::atomic<bool> txError_{false};
    std::atomic<bool> rxNeedsRearm_{false};
    std::atomic<std::size_t> txInFlightSize_{0U};
};

} // namespace platform::middleware::communication