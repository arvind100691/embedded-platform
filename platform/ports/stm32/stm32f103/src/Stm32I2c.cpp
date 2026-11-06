#include "platform/stm32f103/Stm32I2c.hpp"

#include <limits>

namespace platform::stm32f103
{

Stm32I2c::Stm32I2c(I2C_TypeDef* instance)
    : instance_(instance)
{
    handle_.Instance = instance_;
}

platform::Result<void> Stm32I2c::configure(const platform::hal::I2cConfig& config)
{
    constexpr std::uint32_t kMinimumFrequencyHz{10'000U};
    constexpr std::uint32_t kMaximumFrequencyHz{400'000U};
    if (instance_ != I2C1 || config.frequencyHz < kMinimumFrequencyHz ||
        config.frequencyHz > kMaximumFrequencyHz ||
        (config.addressMode != platform::hal::I2cAddressMode::SevenBit &&
         config.addressMode != platform::hal::I2cAddressMode::TenBit))
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    configured_ = false;
    addressMode_ = config.addressMode;
    handle_.Instance = instance_;
    handle_.Init.ClockSpeed = config.frequencyHz;
    handle_.Init.DutyCycle = I2C_DUTYCYCLE_2;
    handle_.Init.OwnAddress1 = 0U;
    handle_.Init.AddressingMode = config.addressMode == platform::hal::I2cAddressMode::SevenBit
                                      ? I2C_ADDRESSINGMODE_7BIT
                                      : I2C_ADDRESSINGMODE_10BIT;
    handle_.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    handle_.Init.OwnAddress2 = 0U;
    handle_.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    handle_.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    const auto status = HAL_I2C_Init(&handle_);
    if (status != HAL_OK)
    {
        return platform::Result<void>::failure(mapHalStatus(status));
    }

    configured_ = true;
    return platform::Result<void>::success();
}

platform::Result<void> Stm32I2c::write(platform::hal::I2cAddress address,
                                       const std::uint8_t* data, std::size_t size,
                                       std::uint32_t timeoutMs)
{
    if (!configured_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::NotReady);
    }
    if (size == 0U)
    {
        return platform::Result<void>::success();
    }
    if (data == nullptr || size > std::numeric_limits<std::uint16_t>::max())
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    std::uint16_t halAddress{0U};
    if (!convertAddress(address, halAddress))
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    const auto status =
        HAL_I2C_Master_Transmit(&handle_, halAddress, const_cast<std::uint8_t*>(data),
                                static_cast<std::uint16_t>(size), timeoutMs);
    if (status != HAL_OK)
    {
        return platform::Result<void>::failure(mapHalStatus(status));
    }
    return platform::Result<void>::success();
}

platform::Result<void> Stm32I2c::read(platform::hal::I2cAddress address, std::uint8_t* data,
                                      std::size_t size, std::uint32_t timeoutMs)
{
    if (!configured_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::NotReady);
    }
    if (size == 0U)
    {
        return platform::Result<void>::success();
    }
    if (data == nullptr || size > std::numeric_limits<std::uint16_t>::max())
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    std::uint16_t halAddress{0U};
    if (!convertAddress(address, halAddress))
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    const auto status = HAL_I2C_Master_Receive(
        &handle_, halAddress, data, static_cast<std::uint16_t>(size), timeoutMs);
    if (status != HAL_OK)
    {
        return platform::Result<void>::failure(mapHalStatus(status));
    }
    return platform::Result<void>::success();
}

platform::Result<void> Stm32I2c::writeRead(platform::hal::I2cAddress address,
                                           const std::uint8_t* txData, std::size_t txSize,
                                           std::uint8_t* rxData, std::size_t rxSize,
                                           std::uint32_t timeoutMs)
{
    if (!configured_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::NotReady);
    }
    if (txData == nullptr || rxData == nullptr || txSize != 1U || rxSize == 0U ||
        rxSize > std::numeric_limits<std::uint16_t>::max())
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    std::uint16_t halAddress{0U};
    if (!convertAddress(address, halAddress))
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    const auto memoryAddress = static_cast<std::uint16_t>(txData[0]);
    const auto status = HAL_I2C_Mem_Read(
        &handle_, halAddress, memoryAddress, I2C_MEMADD_SIZE_8BIT, rxData,
        static_cast<std::uint16_t>(rxSize), timeoutMs);
    if (status != HAL_OK)
    {
        return platform::Result<void>::failure(mapHalStatus(status));
    }
    return platform::Result<void>::success();
}

bool Stm32I2c::convertAddress(platform::hal::I2cAddress address,
                              std::uint16_t& halAddress) const
{
    if (addressMode_ == platform::hal::I2cAddressMode::SevenBit)
    {
        if (address > 0x7FU)
        {
            return false;
        }
        halAddress = static_cast<std::uint16_t>(address << 1U);
        return true;
    }

    if (address > 0x3FFU)
    {
        return false;
    }
    halAddress = address;
    return true;
}

platform::ErrorCode Stm32I2c::mapHalStatus(HAL_StatusTypeDef status)
{
    switch (status)
    {
    case HAL_OK:
        return platform::ErrorCode::Ok;
    case HAL_TIMEOUT:
        return platform::ErrorCode::Timeout;
    case HAL_BUSY:
        return platform::ErrorCode::Busy;
    case HAL_ERROR:
    default:
        return platform::ErrorCode::CommunicationError;
    }
}

} // namespace platform::stm32f103
