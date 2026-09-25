#pragma once

#include <array>
#include <cstdint>

#include "platform/common/Result.hpp"

namespace platform::hal
{

enum class CanIdType
{
    Standard,
    Extended
};

enum class CanFrameType
{
    Data,
    Remote
};

struct CanFrame
{
    std::uint32_t id{0};

    CanIdType idType{
        CanIdType::Standard
    };

    CanFrameType frameType{
        CanFrameType::Data
    };

    std::uint8_t dlc{0};

    std::array<std::uint8_t, 8> data{};
};

struct CanConfig
{
    std::uint32_t bitrate{500'000};
};

class ICan
{
public:
    virtual ~ICan() = default;

    virtual platform::Result<void> configure(
        const CanConfig& config) = 0;

    virtual platform::Result<void> send(
        const CanFrame& frame,
        std::uint32_t timeoutMs) = 0;

    virtual platform::Result<CanFrame> receive(
        std::uint32_t timeoutMs) = 0;
};

} // namespace platform::hal