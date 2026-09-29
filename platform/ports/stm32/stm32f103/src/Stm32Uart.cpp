#include "platform/stm32f103/Stm32Uart.hpp"

#include <limits>

namespace platform::stm32f103
{

Stm32Uart::Stm32Uart(USART_TypeDef* instance)
    : instance_(instance)
{
    handle_.Instance = instance_;
}

platform::Result<void> Stm32Uart::configure(const platform::hal::UartConfig& config)
{
    if (!isValidInstance(instance_) || !isValidConfig(config))
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    handle_.Instance = instance_;
    handle_.Init.BaudRate = config.baudRate;
    handle_.Init.WordLength = toHalWordLength(config);
    handle_.Init.StopBits = toHalStopBits(config.stopBits);
    handle_.Init.Parity = toHalParity(config.parity);
    handle_.Init.Mode = UART_MODE_TX_RX;
    handle_.Init.HwFlowCtl = toHalFlowControl(config.flowControl);
    handle_.Init.OverSampling = UART_OVERSAMPLING_16;

    const HAL_StatusTypeDef status = HAL_UART_Init(&handle_);

    if (status != HAL_OK)
    {
        return platform::Result<void>::failure(mapHalStatus(status));
    }

    return platform::Result<void>::success();
}

platform::Result<void> Stm32Uart::transmit(const std::uint8_t* data, std::size_t size,
                                           std::uint32_t timeoutMs)
{
    if (!isValidInstance(instance_))
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidState);
    }

    if (size == 0U)
    {
        return platform::Result<void>::success();
    }

    if (data == nullptr || size > std::numeric_limits<std::uint16_t>::max())
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    const HAL_StatusTypeDef status =
        HAL_UART_Transmit(&handle_, data, static_cast<std::uint16_t>(size), timeoutMs);

    if (status != HAL_OK)
    {
        return platform::Result<void>::failure(mapHalStatus(status));
    }

    return platform::Result<void>::success();
}

platform::Result<std::size_t> Stm32Uart::receive(std::uint8_t* data, std::size_t size,
                                                 std::uint32_t timeoutMs)
{
    if (!isValidInstance(instance_))
    {
        return platform::Result<std::size_t>::failure(platform::ErrorCode::InvalidState);
    }

    if (size == 0U)
    {
        return platform::Result<std::size_t>::success(0U);
    }

    if (data == nullptr || size > std::numeric_limits<std::uint16_t>::max())
    {
        return platform::Result<std::size_t>::failure(platform::ErrorCode::InvalidArgument);
    }

    const HAL_StatusTypeDef status =
        HAL_UART_Receive(&handle_, data, static_cast<std::uint16_t>(size), timeoutMs);

    if (status != HAL_OK)
    {
        return platform::Result<std::size_t>::failure(mapHalStatus(status));
    }

    return platform::Result<std::size_t>::success(size);
}

bool Stm32Uart::isValidInstance(USART_TypeDef* instance)
{
    return instance == USART1 || instance == USART2 || instance == USART3;
}

bool Stm32Uart::isValidConfig(const platform::hal::UartConfig& config)
{
    if (config.baudRate == 0U)
    {
        return false;
    }

    if (config.dataBits != 8U && config.dataBits != 9U)
    {
        return false;
    }

    /*
     * On STM32F1, an enabled parity bit consumes the MSB of the UART word.
     * Therefore:
     *   8 data bits + parity -> 9-bit HAL word length.
     *   9 data bits + parity -> not representable by this UART abstraction.
     */
    if (config.dataBits == 9U && config.parity != platform::hal::UartParity::None)
    {
        return false;
    }

    return true;
}

std::uint32_t Stm32Uart::toHalWordLength(const platform::hal::UartConfig& config)
{
    if (config.dataBits == 8U && config.parity != platform::hal::UartParity::None)
    {
        return UART_WORDLENGTH_9B;
    }

    return config.dataBits == 9U ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B;
}

std::uint32_t Stm32Uart::toHalParity(platform::hal::UartParity parity)
{
    switch (parity)
    {
    case platform::hal::UartParity::Even:
        return UART_PARITY_EVEN;

    case platform::hal::UartParity::Odd:
        return UART_PARITY_ODD;

    case platform::hal::UartParity::None:
    default:
        return UART_PARITY_NONE;
    }
}

std::uint32_t Stm32Uart::toHalStopBits(platform::hal::UartStopBits stopBits)
{
    switch (stopBits)
    {
    case platform::hal::UartStopBits::Two:
        return UART_STOPBITS_2;

    case platform::hal::UartStopBits::One:
    default:
        return UART_STOPBITS_1;
    }
}

std::uint32_t Stm32Uart::toHalFlowControl(platform::hal::UartFlowControl flowControl)
{
    switch (flowControl)
    {
    case platform::hal::UartFlowControl::RtsCts:
        return UART_HWCONTROL_RTS_CTS;

    case platform::hal::UartFlowControl::None:
    default:
        return UART_HWCONTROL_NONE;
    }
}

platform::ErrorCode Stm32Uart::mapHalStatus(HAL_StatusTypeDef status)
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
