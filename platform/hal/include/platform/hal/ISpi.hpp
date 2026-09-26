#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::hal
{

enum class SpiMode
{
    Mode0,
    Mode1,
    Mode2,
    Mode3
};

enum class SpiBitOrder
{
    MsbFirst,
    LsbFirst
};

struct SpiConfig
{
    std::uint32_t frequencyHz{1'000'000};

    SpiMode mode{SpiMode::Mode0};

    SpiBitOrder bitOrder{SpiBitOrder::MsbFirst};

    std::uint8_t dataBits{8};
};

class ISpi
{
public:
    virtual ~ISpi() = default;

    virtual platform::Result<void> configure(const SpiConfig& config) = 0;

    virtual platform::Result<void> transmit(const std::uint8_t* data, std::size_t size,
                                            std::uint32_t timeoutMs) = 0;

    virtual platform::Result<void> receive(std::uint8_t* data, std::size_t size,
                                           std::uint32_t timeoutMs) = 0;

    virtual platform::Result<void> transfer(const std::uint8_t* txData, std::uint8_t* rxData,
                                            std::size_t size, std::uint32_t timeoutMs) = 0;
};

} // namespace platform::hal