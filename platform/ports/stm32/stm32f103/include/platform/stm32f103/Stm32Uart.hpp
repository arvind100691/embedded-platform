#pragma once

#include <cstdint>

#include "platform/common/NonCopyable.hpp"
#include "platform/hal/IUart.hpp"

#include "stm32f1xx_hal.h"

namespace platform::stm32f103
{

class Stm32Uart final : public platform::hal::IUart, public platform::NonCopyable
{
public:
    explicit Stm32Uart(USART_TypeDef* instance);

    ~Stm32Uart() override = default;

    platform::Result<void> configure(const platform::hal::UartConfig& config) override;

    platform::Result<void> transmit(const std::uint8_t* data, std::size_t size,
                                    std::uint32_t timeoutMs) override;

    platform::Result<std::size_t> receive(std::uint8_t* data, std::size_t size,
                                          std::uint32_t timeoutMs) override;

private:
    static bool isValidInstance(USART_TypeDef* instance);
    static bool isValidConfig(const platform::hal::UartConfig& config);

    static std::uint32_t toHalWordLength(const platform::hal::UartConfig& config);
    static std::uint32_t toHalParity(platform::hal::UartParity parity);
    static std::uint32_t toHalStopBits(platform::hal::UartStopBits stopBits);
    static std::uint32_t toHalFlowControl(platform::hal::UartFlowControl flowControl);

    static platform::ErrorCode mapHalStatus(HAL_StatusTypeDef status);

private:
    USART_TypeDef* instance_;
    UART_HandleTypeDef handle_{};
};

} // namespace platform::stm32f103
