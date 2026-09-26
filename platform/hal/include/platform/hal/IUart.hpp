#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::hal
{

enum class UartParity
{
    None,
    Even,
    Odd
};

enum class UartStopBits
{
    One,
    Two
};

enum class UartFlowControl
{
    None,
    RtsCts
};

struct UartConfig
{
    std::uint32_t baudRate{115200};

    std::uint8_t dataBits{8};

    UartParity parity{UartParity::None};

    UartStopBits stopBits{UartStopBits::One};

    UartFlowControl flowControl{UartFlowControl::None};
};

class IUart
{
public:
    virtual ~IUart() = default;

    virtual platform::Result<void> configure(const UartConfig& config) = 0;

    virtual platform::Result<void> transmit(const std::uint8_t* data, std::size_t size,
                                            std::uint32_t timeoutMs) = 0;

    virtual platform::Result<std::size_t> receive(std::uint8_t* data, std::size_t size,
                                                  std::uint32_t timeoutMs) = 0;
};

} // namespace platform::hal