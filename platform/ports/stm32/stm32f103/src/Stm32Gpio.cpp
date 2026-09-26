#include "platform/stm32f103/Stm32Gpio.hpp"

#include <array>

namespace
{

struct CallbackEntry
{
    platform::hal::GpioInterruptCallback callback{nullptr};
    void* context{nullptr};
};

std::array<CallbackEntry, 16> g_callbacks{};

} // namespace

namespace platform::stm32f103
{

Stm32Gpio::Stm32Gpio(GPIO_TypeDef* port, std::uint16_t pin)
    : port_(port)
    , pin_(pin)
{
}

Stm32Gpio::~Stm32Gpio()
{
    const int index = pinToIndex(pin_);

    if (index >= 0)
    {
        g_callbacks[static_cast<std::size_t>(index)] = {};
    }
}

platform::Result<void> Stm32Gpio::configure(const platform::hal::GpioConfig& config)
{
    GPIO_InitTypeDef gpioInit{};

    gpioInit.Pin = pin_;

    gpioInit.Mode = toHalMode(config);

    gpioInit.Pull = toHalPull(config.pull);

    gpioInit.Speed = GPIO_SPEED_FREQ_LOW;

    if (config.direction == platform::hal::GpioDirection::Output)
    {
        HAL_GPIO_WritePin(port_, pin_,
                          config.initialState == platform::hal::GpioState::High ? GPIO_PIN_SET
                                                                                : GPIO_PIN_RESET);
    }

    HAL_GPIO_Init(port_, &gpioInit);

    return platform::Result<void>::success();
}

platform::Result<void> Stm32Gpio::write(platform::hal::GpioState state)
{
    HAL_GPIO_WritePin(port_, pin_,
                      state == platform::hal::GpioState::High ? GPIO_PIN_SET : GPIO_PIN_RESET);

    return platform::Result<void>::success();
}

platform::Result<platform::hal::GpioState> Stm32Gpio::read() const
{
    const GPIO_PinState state = HAL_GPIO_ReadPin(port_, pin_);

    return platform::Result<platform::hal::GpioState>::success(
        state == GPIO_PIN_SET ? platform::hal::GpioState::High : platform::hal::GpioState::Low);
}

platform::Result<void> Stm32Gpio::toggle()
{
    HAL_GPIO_TogglePin(port_, pin_);

    return platform::Result<void>::success();
}

platform::Result<void>
Stm32Gpio::registerInterruptCallback(platform::hal::GpioInterruptCallback callback, void* context)
{
    const int index = pinToIndex(pin_);

    if (index < 0)
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    auto& entry = g_callbacks[static_cast<std::size_t>(index)];

    entry.callback = callback;
    entry.context = context;

    return platform::Result<void>::success();
}

platform::Result<void> Stm32Gpio::enableInterrupt()
{
    if (pinToIndex(pin_) < 0)
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    __HAL_RCC_AFIO_CLK_ENABLE();

    const auto result = enableIrq(pin_);

    if (result != platform::ErrorCode::Ok)
    {
        return platform::Result<void>::failure(result);
    }

    return platform::Result<void>::success();
}

platform::Result<void> Stm32Gpio::disableInterrupt()
{
    const auto result = disableIrq(pin_);

    if (result != platform::ErrorCode::Ok)
    {
        return platform::Result<void>::failure(result);
    }

    return platform::Result<void>::success();
}

int Stm32Gpio::pinToIndex(std::uint16_t pin)
{
    switch (pin)
    {
    case GPIO_PIN_0:
        return 0;
    case GPIO_PIN_1:
        return 1;
    case GPIO_PIN_2:
        return 2;
    case GPIO_PIN_3:
        return 3;
    case GPIO_PIN_4:
        return 4;
    case GPIO_PIN_5:
        return 5;
    case GPIO_PIN_6:
        return 6;
    case GPIO_PIN_7:
        return 7;
    case GPIO_PIN_8:
        return 8;
    case GPIO_PIN_9:
        return 9;
    case GPIO_PIN_10:
        return 10;
    case GPIO_PIN_11:
        return 11;
    case GPIO_PIN_12:
        return 12;
    case GPIO_PIN_13:
        return 13;
    case GPIO_PIN_14:
        return 14;
    case GPIO_PIN_15:
        return 15;

    default:
        return -1;
    }
}

std::uint32_t Stm32Gpio::toHalMode(const platform::hal::GpioConfig& config)
{
    using Direction = platform::hal::GpioDirection;

    using Edge = platform::hal::GpioInterruptEdge;

    if (config.direction == Direction::Output)
    {
        return GPIO_MODE_OUTPUT_PP;
    }

    switch (config.interruptEdge)
    {
    case Edge::Rising:
        return GPIO_MODE_IT_RISING;

    case Edge::Falling:
        return GPIO_MODE_IT_FALLING;

    case Edge::Both:
        return GPIO_MODE_IT_RISING_FALLING;

    case Edge::None:
    default:
        return GPIO_MODE_INPUT;
    }
}

std::uint32_t Stm32Gpio::toHalPull(platform::hal::GpioPull pull)
{
    switch (pull)
    {
    case platform::hal::GpioPull::PullUp:
        return GPIO_PULLUP;

    case platform::hal::GpioPull::PullDown:
        return GPIO_PULLDOWN;

    case platform::hal::GpioPull::None:
    default:
        return GPIO_NOPULL;
    }
}

platform::ErrorCode Stm32Gpio::enableIrq(std::uint16_t pin)
{
    switch (pin)
    {
    case GPIO_PIN_0:
        HAL_NVIC_EnableIRQ(EXTI0_IRQn);
        return platform::ErrorCode::Ok;

    case GPIO_PIN_1:
        HAL_NVIC_EnableIRQ(EXTI1_IRQn);
        return platform::ErrorCode::Ok;

    case GPIO_PIN_2:
        HAL_NVIC_EnableIRQ(EXTI2_IRQn);
        return platform::ErrorCode::Ok;

    case GPIO_PIN_3:
        HAL_NVIC_EnableIRQ(EXTI3_IRQn);
        return platform::ErrorCode::Ok;

    case GPIO_PIN_4:
        HAL_NVIC_EnableIRQ(EXTI4_IRQn);
        return platform::ErrorCode::Ok;

    case GPIO_PIN_5:
    case GPIO_PIN_6:
    case GPIO_PIN_7:
    case GPIO_PIN_8:
    case GPIO_PIN_9:
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
        return platform::ErrorCode::Ok;

    case GPIO_PIN_10:
    case GPIO_PIN_11:
    case GPIO_PIN_12:
    case GPIO_PIN_13:
    case GPIO_PIN_14:
    case GPIO_PIN_15:
        HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
        return platform::ErrorCode::Ok;

    default:
        return platform::ErrorCode::InvalidArgument;
    }
}

platform::ErrorCode Stm32Gpio::disableIrq(std::uint16_t pin)
{
    switch (pin)
    {
    case GPIO_PIN_0:
        HAL_NVIC_DisableIRQ(EXTI0_IRQn);
        return platform::ErrorCode::Ok;

    case GPIO_PIN_1:
        HAL_NVIC_DisableIRQ(EXTI1_IRQn);
        return platform::ErrorCode::Ok;

    case GPIO_PIN_2:
        HAL_NVIC_DisableIRQ(EXTI2_IRQn);
        return platform::ErrorCode::Ok;

    case GPIO_PIN_3:
        HAL_NVIC_DisableIRQ(EXTI3_IRQn);
        return platform::ErrorCode::Ok;

    case GPIO_PIN_4:
        HAL_NVIC_DisableIRQ(EXTI4_IRQn);
        return platform::ErrorCode::Ok;

    case GPIO_PIN_5:
    case GPIO_PIN_6:
    case GPIO_PIN_7:
    case GPIO_PIN_8:
    case GPIO_PIN_9:
        HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
        return platform::ErrorCode::Ok;

    case GPIO_PIN_10:
    case GPIO_PIN_11:
    case GPIO_PIN_12:
    case GPIO_PIN_13:
    case GPIO_PIN_14:
    case GPIO_PIN_15:
        HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
        return platform::ErrorCode::Ok;

    default:
        return platform::ErrorCode::InvalidArgument;
    }
}

void Stm32Gpio::handleExti(std::uint16_t pin)
{
    const int index = pinToIndex(pin);

    if (index < 0)
    {
        return;
    }

    const auto& entry = g_callbacks[static_cast<std::size_t>(index)];

    if (entry.callback != nullptr)
    {
        entry.callback(entry.context);
    }
}

} // namespace platform::stm32f103

extern "C" void HAL_GPIO_EXTI_Callback(std::uint16_t GPIO_Pin)
{
    platform::stm32f103::Stm32Gpio::handleExti(GPIO_Pin);
}