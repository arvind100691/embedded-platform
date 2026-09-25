#pragma once

#include <cstdint>

namespace platform
{

enum class ErrorCode : std::uint16_t
{
    Ok = 0,

    InvalidArgument,
    InvalidState,

    Timeout,
    Busy,
    NotReady,

    NotFound,

    BufferFull,
    BufferEmpty,

    CommunicationError,
    StorageError,
    HardwareFault,

    Unsupported,

    InternalError
};

constexpr bool isSuccess(ErrorCode error) noexcept
{
    return error == ErrorCode::Ok;
}

} // namespace platform