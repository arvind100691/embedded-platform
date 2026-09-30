#pragma once

#include <array>
#include <atomic>
#include <cstddef>

#include "platform/common/Types.hpp"

namespace platform
{

template <std::size_t Capacity> class SpscByteRingBuffer
{
    static_assert(Capacity > 0U, "SpscByteRingBuffer capacity must be greater than zero");
    static_assert(std::atomic<std::size_t>::is_always_lock_free,
                  "SpscByteRingBuffer requires lock-free size_t atomics");

public:
    [[nodiscard]] constexpr std::size_t capacity() const noexcept
    {
        return Capacity;
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t used = head - tail;
        return (used < Capacity) ? used : Capacity;
    }

    [[nodiscard]] std::size_t freeSpace() const noexcept
    {
        return Capacity - size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return size() == 0U;
    }

    [[nodiscard]] bool full() const noexcept
    {
        return freeSpace() == 0U;
    }

    bool push(platform::Byte value) noexcept
    {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_acquire);

        if ((head - tail) >= Capacity)
        {
            return false;
        }

        buffer_[physicalIndex(head)] = value;
        head_.store(head + 1U, std::memory_order_release);
        return true;
    }

    bool pop(platform::Byte& value) noexcept
    {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t head = head_.load(std::memory_order_acquire);

        if (tail == head)
        {
            return false;
        }

        value = buffer_[physicalIndex(tail)];
        tail_.store(tail + 1U, std::memory_order_release);
        return true;
    }

    std::size_t write(const platform::Byte* data, std::size_t length) noexcept
    {
        if (data == nullptr || length == 0U)
        {
            return 0U;
        }

        std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        const std::size_t used = head - tail;
        const std::size_t available = (used < Capacity) ? Capacity - used : 0U;
        const std::size_t count = (length < available) ? length : available;

        for (std::size_t index = 0U; index < count; ++index)
        {
            buffer_[physicalIndex(head)] = data[index];
            ++head;
        }

        head_.store(head, std::memory_order_release);
        return count;
    }

    std::size_t read(platform::Byte* data, std::size_t length) noexcept
    {
        if (data == nullptr || length == 0U)
        {
            return 0U;
        }

        std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t available = head - tail;
        const std::size_t count = (length < available) ? length : available;

        for (std::size_t index = 0U; index < count; ++index)
        {
            data[index] = buffer_[physicalIndex(tail)];
            ++tail;
        }

        tail_.store(tail, std::memory_order_release);
        return count;
    }

    std::size_t peek(platform::Byte* data, std::size_t length) const noexcept
    {
        if (data == nullptr || length == 0U)
        {
            return 0U;
        }

        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t available = head - tail;
        const std::size_t count = (length < available) ? length : available;

        for (std::size_t index = 0U; index < count; ++index)
        {
            data[index] = buffer_[physicalIndex(tail + index)];
        }

        return count;
    }

    bool discard(std::size_t length) noexcept
    {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t head = head_.load(std::memory_order_acquire);

        if (length > (head - tail))
        {
            return false;
        }

        tail_.store(tail + length, std::memory_order_release);
        return true;
    }

    void clear() noexcept
    {
        head_.store(0U, std::memory_order_relaxed);
        tail_.store(0U, std::memory_order_relaxed);
    }

private:
    static constexpr std::size_t physicalIndex(std::size_t index) noexcept
    {
        return index % Capacity;
    }

    std::array<platform::Byte, Capacity> buffer_{};
    std::atomic<std::size_t> head_{0U};
    std::atomic<std::size_t> tail_{0U};
};

} // namespace platform