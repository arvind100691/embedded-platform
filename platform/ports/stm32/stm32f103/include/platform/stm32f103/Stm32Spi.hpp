#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/common/NonCopyable.hpp"
#include "platform/hal/ISpi.hpp"

#include "stm32f1xx_hal.h"

namespace platform::stm32f103
{

class Stm32Spi final : public platform::hal::ISpi, public platform::NonCopyable
{
public:
    explicit Stm32Spi(SPI_TypeDef* instance);

    platform::Result<void> configure(const platform::hal::SpiConfig& config) override;

    platform::Result<void> transmit(const std::uint8_t* data, std::size_t size,
                                    std::uint32_t timeoutMs) override;

    platform::Result<void> receive(std::uint8_t* data, std::size_t size,
                                   std::uint32_t timeoutMs) override;

    platform::Result<void> transfer(const std::uint8_t* txData, std::uint8_t* rxData,
                                    std::size_t size, std::uint32_t timeoutMs) override;

private:
    static bool isValidInstance(SPI_TypeDef* instance);
    static bool isValidConfig(const platform::hal::SpiConfig& config);
    static bool toHalMode(platform::hal::SpiMode mode, std::uint32_t& polarity,
                          std::uint32_t& phase);
    static bool toHalBitOrder(platform::hal::SpiBitOrder bitOrder, std::uint32_t& firstBit);
    static bool toHalPrescaler(std::uint32_t peripheralClockHz, std::uint32_t requestedHz,
                               std::uint32_t& prescaler);
    static platform::ErrorCode mapHalStatus(HAL_StatusTypeDef status);

private:
    SPI_TypeDef* instance_;
    SPI_HandleTypeDef handle_{};
    bool configured_{false};
};

} // namespace platform::stm32f103