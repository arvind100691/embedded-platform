#pragma once

#include <cstdint>

#include "stm32f1xx_hal.h"

#include "platform/common/NonCopyable.hpp"
#include "platform/hal/IGpio.hpp"

namespace platform::stm32f103
{

class Stm32Gpio final : public platform::hal::IGpio,
                        public platform::NonCopyable
{
public:
    Stm32Gpio(
        GPIO_TypeDef* port,
        std::uint16_t pin);

    ~Stm32Gpio() override;

    platform::Result<void> configure(
        const platform::hal::GpioConfig& config) override;

    platform::Result<void> write(
        platform::hal::GpioState state) override;

    platform::Result<platform::hal::GpioState> read() const override;

    platform::Result<void> toggle() override;

    platform::Result<void> registerInterruptCallback(
        platform::hal::GpioInterruptCallback callback,
        void* context) override;

    platform::Result<void> enableInterrupt() override;

    platform::Result<void> disableInterrupt() override;

    static void handleExti(
        std::uint16_t pin);

private:
    static int pinToIndex(
        std::uint16_t pin);

    static std::uint32_t toHalMode(
        const platform::hal::GpioConfig& config);

    static std::uint32_t toHalPull(
        platform::hal::GpioPull pull);

    static platform::ErrorCode enableIrq(
        std::uint16_t pin);

    static platform::ErrorCode disableIrq(
        std::uint16_t pin);

private:
    GPIO_TypeDef* port_;
    std::uint16_t pin_;
};

} // namespace platform::stm32f103