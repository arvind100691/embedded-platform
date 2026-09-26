#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::hal
{

enum class UartTransferStatus
{
    Completed,
    Aborted,
    Error
};

using UartTransferCallback = void (*)(UartTransferStatus status, std::size_t transferredBytes,
                                      void* context);

class IAsyncUart
{
public:
    virtual ~IAsyncUart() = default;

    virtual platform::Result<void> transmitAsync(const std::uint8_t* data, std::size_t size,
                                                 UartTransferCallback callback, void* context) = 0;

    virtual platform::Result<void> receiveAsync(std::uint8_t* data, std::size_t size,
                                                UartTransferCallback callback, void* context) = 0;

    virtual platform::Result<void> cancelTransmit() = 0;

    virtual platform::Result<void> cancelReceive() = 0;
};

} // namespace platform::hal