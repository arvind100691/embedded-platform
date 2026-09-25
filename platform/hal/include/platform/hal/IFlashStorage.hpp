#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::hal
{

struct FlashInfo
{
    std::uint32_t startAddress{0};

    std::uint32_t size{0};

    std::uint32_t eraseBlockSize{0};

    std::uint32_t programUnitSize{0};
};

class IFlashStorage
{
public:
    virtual ~IFlashStorage() = default;

    virtual platform::Result<FlashInfo> getInfo() const = 0;

    virtual platform::Result<void> erase(
        std::uint32_t address,
        std::size_t size) = 0;

    virtual platform::Result<void> write(
        std::uint32_t address,
        const std::uint8_t* data,
        std::size_t size) = 0;

    virtual platform::Result<void> read(
        std::uint32_t address,
        std::uint8_t* data,
        std::size_t size) = 0;
};

} // namespace platform::hal