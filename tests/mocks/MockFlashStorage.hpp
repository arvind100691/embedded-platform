#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "platform/hal/IFlashStorage.hpp"

class MockFlashStorage final : public platform::hal::IFlashStorage
{
public:
    platform::Result<platform::hal::FlashInfo> getInfo() const override
    {
        return platform::Result<platform::hal::FlashInfo>::success(info_);
    }

    platform::Result<void> erase(std::uint32_t address, std::size_t size) override
    {
        ++eraseCallCount_;
        lastEraseAddress_ = address;
        lastEraseSize_ = size;
        return platform::Result<void>::success();
    }

    platform::Result<void> write(std::uint32_t address, const std::uint8_t* data,
                                 std::size_t size) override
    {
        ++writeCallCount_;
        lastWriteAddress_ = address;
        writtenData_.assign(data, data + size);
        return platform::Result<void>::success();
    }

    platform::Result<void> read(std::uint32_t address, std::uint8_t* data,
                                std::size_t size) override
    {
        ++readCallCount_;
        lastReadAddress_ = address;
        const std::size_t count = std::min(size, readData_.size());
        std::copy_n(readData_.begin(), count, data);
        return platform::Result<void>::success();
    }

    void setReadData(const std::vector<std::uint8_t>& data)
    {
        readData_ = data;
    }

    [[nodiscard]] std::size_t readCallCount() const
    {
        return readCallCount_;
    }
    [[nodiscard]] std::size_t writeCallCount() const
    {
        return writeCallCount_;
    }
    [[nodiscard]] std::size_t eraseCallCount() const
    {
        return eraseCallCount_;
    }
    [[nodiscard]] std::uint32_t lastReadAddress() const
    {
        return lastReadAddress_;
    }
    [[nodiscard]] std::uint32_t lastWriteAddress() const
    {
        return lastWriteAddress_;
    }
    [[nodiscard]] std::uint32_t lastEraseAddress() const
    {
        return lastEraseAddress_;
    }
    [[nodiscard]] std::size_t lastEraseSize() const
    {
        return lastEraseSize_;
    }
    [[nodiscard]] const std::vector<std::uint8_t>& writtenData() const
    {
        return writtenData_;
    }

private:
    platform::hal::FlashInfo info_{0U, 16U * 1024U * 1024U, 4096U, 1U};
    std::vector<std::uint8_t> readData_{};
    std::vector<std::uint8_t> writtenData_{};
    std::size_t readCallCount_{0U};
    std::size_t writeCallCount_{0U};
    std::size_t eraseCallCount_{0U};
    std::uint32_t lastReadAddress_{0U};
    std::uint32_t lastWriteAddress_{0U};
    std::uint32_t lastEraseAddress_{0U};
    std::size_t lastEraseSize_{0U};
};