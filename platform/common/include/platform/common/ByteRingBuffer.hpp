#pragma once

#include <array>
#include <cstddef>

#include "platform/common/Types.hpp"

namespace platform
{

template <std::size_t Capacity> class ByteRingBuffer
{
    static_assert(Capacity > 0U, "ByteRingBuffer capacity must be greater than zero");

public:
    [[nodiscard]] constexpr std::size_t capacity() const noexcept
    {
        return Capacity;
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return size_;
    }

    [[nodiscard]] std::size_t freeSpace() const noexcept
    {
        return Capacity - size_;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return size_ == 0U;
    }

    [[nodiscard]] bool full() const noexcept
    {
        return size_ == Capacity;
    }

    void clear() noexcept
    {
        head_ = 0U;
        tail_ = 0U;
        size_ = 0U;
    }

    bool push(platform::Byte value) noexcept
    {
        if (full())
        {
            return false;
        }

        buffer_[head_] = value;
        head_ = nextIndex(head_);
        ++size_;

        return true;
    }

    bool pop(platform::Byte& value) noexcept
    {
        if (empty())
        {
            return false;
        }

        value = buffer_[tail_];
        tail_ = nextIndex(tail_);
        --size_;

        return true;
    }

    std::size_t write(const platform::Byte* data, std::size_t length) noexcept
    {
        if (data == nullptr || length == 0U)
        {
            return 0U;
        }

        const std::size_t count = (length < freeSpace()) ? length : freeSpace();

        for (std::size_t index = 0U; index < count; ++index)
        {
            buffer_[head_] = data[index];
            head_ = nextIndex(head_);
        }

        size_ += count;
        return count;
    }

    std::size_t read(platform::Byte* data, std::size_t length) noexcept
    {
        if (data == nullptr || length == 0U)
        {
            return 0U;
        }

        const std::size_t count = (length < size_) ? length : size_;

        for (std::size_t index = 0U; index < count; ++index)
        {
            data[index] = buffer_[tail_];
            tail_ = nextIndex(tail_);
        }

        size_ -= count;
        return count;
    }

private:
    static constexpr std::size_t nextIndex(std::size_t index) noexcept
    {
        ++index;
        return (index == Capacity) ? 0U : index;
    }

private:
    std::array<platform::Byte, Capacity> buffer_{};
    std::size_t head_{0U};
    std::size_t tail_{0U};
    std::size_t size_{0U};
};

} // namespace platform
