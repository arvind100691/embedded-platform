#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::hal
{

enum class I2cAddressMode
{
    SevenBit,
    TenBit
};

struct I2cConfig
{
    std::uint32_t frequencyHz{100'000};

    I2cAddressMode addressMode{I2cAddressMode::SevenBit};
};

using I2cAddress = std::uint16_t;

class II2c
{
public:
    virtual ~II2c() = default;

    virtual platform::Result<void> configure(const I2cConfig& config) = 0;

    virtual platform::Result<void> write(I2cAddress address, const std::uint8_t* data,
                                         std::size_t size, std::uint32_t timeoutMs) = 0;

    virtual platform::Result<void> read(I2cAddress address, std::uint8_t* data, std::size_t size,
                                        std::uint32_t timeoutMs) = 0;

    virtual platform::Result<void> writeRead(I2cAddress address, const std::uint8_t* txData,
                                             std::size_t txSize, std::uint8_t* rxData,
                                             std::size_t rxSize, std::uint32_t timeoutMs) = 0;
};

} // namespace platform::hal