#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/common/NonCopyable.hpp"
#include "platform/hal/II2c.hpp"

#include "stm32f1xx_hal.h"

namespace platform::stm32f103
{

class Stm32I2c final : public platform::hal::II2c, public platform::NonCopyable
{
public:
    explicit Stm32I2c(I2C_TypeDef* instance);

    platform::Result<void> configure(const platform::hal::I2cConfig& config) override;

    platform::Result<void> write(platform::hal::I2cAddress address, const std::uint8_t* data,
                                 std::size_t size, std::uint32_t timeoutMs) override;

    platform::Result<void> read(platform::hal::I2cAddress address, std::uint8_t* data,
                                std::size_t size, std::uint32_t timeoutMs) override;

    platform::Result<void> writeRead(platform::hal::I2cAddress address,
                                     const std::uint8_t* txData, std::size_t txSize,
                                     std::uint8_t* rxData, std::size_t rxSize,
                                     std::uint32_t timeoutMs) override;

private:
    bool convertAddress(platform::hal::I2cAddress address, std::uint16_t& halAddress) const;
    static platform::ErrorCode mapHalStatus(HAL_StatusTypeDef status);

    I2C_TypeDef* instance_;
    I2C_HandleTypeDef handle_{};
    platform::hal::I2cAddressMode addressMode_{platform::hal::I2cAddressMode::SevenBit};
    bool configured_{false};
};

} // namespace platform::stm32f103
