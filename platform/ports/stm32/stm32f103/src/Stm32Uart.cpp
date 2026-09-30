#include "platform/stm32f103/Stm32Uart.hpp"

#include <limits>

namespace
{

constexpr std::size_t kMaxUartInstances = 3U;

platform::stm32f103::Stm32Uart* g_uartInstances[kMaxUartInstances]{};

} // namespace

namespace platform::stm32f103
{

Stm32Uart::Stm32Uart(USART_TypeDef* instance)
    : instance_(instance)
{
    handle_.Instance = instance_;
}

Stm32Uart::~Stm32Uart()
{
    unregisterInstance();
}

platform::Result<void> Stm32Uart::configure(const platform::hal::UartConfig& config)
{
    if (!isValidInstance(instance_) || !isValidConfig(config))
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    if (txActive_ || rxActive_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::Busy);
    }

    if (!registered_ && !registerInstance())
    {
        return platform::Result<void>::failure(platform::ErrorCode::Busy);
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
        if (!configured_)
        {
            unregisterInstance();
        }

        return platform::Result<void>::failure(mapHalStatus(status));
    }

    configured_ = true;
    return platform::Result<void>::success();
}

platform::Result<void> Stm32Uart::transmit(const std::uint8_t* data, std::size_t size,
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
    if (!configured_)
    {
        return platform::Result<std::size_t>::failure(platform::ErrorCode::NotReady);
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

platform::Result<void> Stm32Uart::transmitAsync(const std::uint8_t* data, std::size_t size,
                                                platform::hal::UartTransferCallback callback,
                                                void* context)
{
    if (!configured_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::NotReady);
    }

    if (callback == nullptr || data == nullptr || size == 0U ||
        size > std::numeric_limits<std::uint16_t>::max())
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    if (txActive_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::Busy);
    }

    txRequestedSize_ = static_cast<std::uint16_t>(size);
    txCallback_ = callback;
    txContext_ = context;
    txAbortTransferred_ = 0U;
    txActive_ = true;

    const HAL_StatusTypeDef status =
        HAL_UART_Transmit_IT(&handle_, data, static_cast<std::uint16_t>(size));

    if (status != HAL_OK)
    {
        clearTransmitCallbackState();
        return platform::Result<void>::failure(mapHalStatus(status));
    }

    return platform::Result<void>::success();
}

platform::Result<void> Stm32Uart::receiveAsync(std::uint8_t* data, std::size_t size,
                                               platform::hal::UartTransferCallback callback,
                                               void* context)
{
    if (!configured_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::NotReady);
    }

    if (callback == nullptr || data == nullptr || size == 0U ||
        size > std::numeric_limits<std::uint16_t>::max())
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    if (rxActive_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::Busy);
    }

    rxRequestedSize_ = static_cast<std::uint16_t>(size);
    rxCallback_ = callback;
    rxContext_ = context;
    rxAbortTransferred_ = 0U;
    rxActive_ = true;

    const HAL_StatusTypeDef status =
        HAL_UART_Receive_IT(&handle_, data, static_cast<std::uint16_t>(size));

    if (status != HAL_OK)
    {
        clearReceiveCallbackState();
        return platform::Result<void>::failure(mapHalStatus(status));
    }

    return platform::Result<void>::success();
}

platform::Result<void> Stm32Uart::cancelTransmit()
{
    if (!configured_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::NotReady);
    }

    if (!txActive_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::NotReady);
    }

    txAbortTransferred_ = transferredBytes(txRequestedSize_, handle_.TxXferCount);

    const HAL_StatusTypeDef status = HAL_UART_AbortTransmit_IT(&handle_);

    if (status != HAL_OK)
    {
        return platform::Result<void>::failure(mapHalStatus(status));
    }

    return platform::Result<void>::success();
}

platform::Result<void> Stm32Uart::cancelReceive()
{
    if (!configured_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::NotReady);
    }

    if (!rxActive_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::NotReady);
    }

    rxAbortTransferred_ = transferredBytes(rxRequestedSize_, handle_.RxXferCount);

    const HAL_StatusTypeDef status = HAL_UART_AbortReceive_IT(&handle_);

    if (status != HAL_OK)
    {
        return platform::Result<void>::failure(mapHalStatus(status));
    }

    return platform::Result<void>::success();
}

void Stm32Uart::handleTxComplete()
{
    if (!txActive_)
    {
        return;
    }

    const auto callback = txCallback_;
    void* const context = txContext_;
    const std::size_t transferred = txRequestedSize_;

    clearTransmitCallbackState();

    if (callback != nullptr)
    {
        callback(platform::hal::UartTransferStatus::Completed, transferred, context);
    }
}

void Stm32Uart::handleRxComplete()
{
    if (!rxActive_)
    {
        return;
    }

    const auto callback = rxCallback_;
    void* const context = rxContext_;
    const std::size_t transferred = rxRequestedSize_;

    clearReceiveCallbackState();

    if (callback != nullptr)
    {
        callback(platform::hal::UartTransferStatus::Completed, transferred, context);
    }
}

void Stm32Uart::handleError()
{
    if (!rxActive_)
    {
        return;
    }

    const auto rxCallback = rxCallback_;
    void* const rxContext = rxContext_;
    const std::size_t rxTransferred = transferredBytes(rxRequestedSize_, handle_.RxXferCount);

    clearReceiveCallbackState();

    if (rxCallback != nullptr)
    {
        rxCallback(platform::hal::UartTransferStatus::Error, rxTransferred, rxContext);
    }
}

void Stm32Uart::handleAbortTransmitComplete()
{
    if (!txActive_)
    {
        return;
    }

    const auto callback = txCallback_;
    void* const context = txContext_;
    const std::size_t transferred = txAbortTransferred_;

    clearTransmitCallbackState();

    if (callback != nullptr)
    {
        callback(platform::hal::UartTransferStatus::Aborted, transferred, context);
    }
}

void Stm32Uart::handleAbortReceiveComplete()
{
    if (!rxActive_)
    {
        return;
    }

    const auto callback = rxCallback_;
    void* const context = rxContext_;
    const std::size_t transferred = rxAbortTransferred_;

    clearReceiveCallbackState();

    if (callback != nullptr)
    {
        callback(platform::hal::UartTransferStatus::Aborted, transferred, context);
    }
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

Stm32Uart* Stm32Uart::findByInstance(USART_TypeDef* instance)
{
    for (std::size_t index = 0U; index < kMaxUartInstances; ++index)
    {
        if (g_uartInstances[index] != nullptr && g_uartInstances[index]->instance_ == instance)
        {
            return g_uartInstances[index];
        }
    }

    return nullptr;
}

Stm32Uart* Stm32Uart::findByHandle(UART_HandleTypeDef* handle)
{
    if (handle == nullptr)
    {
        return nullptr;
    }

    return findByInstance(handle->Instance);
}

void Stm32Uart::irqHandler(USART_TypeDef* instance)
{
    if (auto* uart = findByInstance(instance))
    {
        HAL_UART_IRQHandler(&uart->handle_);
    }
}

bool Stm32Uart::registerInstance()
{
    if (!isValidInstance(instance_))
    {
        return false;
    }

    if (registered_)
    {
        return true;
    }

    if (findByInstance(instance_) != nullptr)
    {
        return false;
    }

    for (std::size_t index = 0U; index < kMaxUartInstances; ++index)
    {
        if (g_uartInstances[index] == nullptr)
        {
            g_uartInstances[index] = this;
            registered_ = true;
            return true;
        }
    }

    return false;
}

void Stm32Uart::unregisterInstance()
{
    if (!registered_)
    {
        return;
    }

    for (std::size_t index = 0U; index < kMaxUartInstances; ++index)
    {
        if (g_uartInstances[index] == this)
        {
            g_uartInstances[index] = nullptr;
            break;
        }
    }

    registered_ = false;
}

std::size_t Stm32Uart::transferredBytes(std::uint16_t requested, std::uint16_t remaining)
{
    if (remaining > requested)
    {
        return 0U;
    }

    return static_cast<std::size_t>(requested - remaining);
}

void Stm32Uart::clearTransmitCallbackState()
{
    txActive_ = false;
    txRequestedSize_ = 0U;
    txCallback_ = nullptr;
    txContext_ = nullptr;
    txAbortTransferred_ = 0U;
}

void Stm32Uart::clearReceiveCallbackState()
{
    rxActive_ = false;
    rxRequestedSize_ = 0U;
    rxCallback_ = nullptr;
    rxContext_ = nullptr;
    rxAbortTransferred_ = 0U;
}

} // namespace platform::stm32f103

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    if (auto* uart = platform::stm32f103::Stm32Uart::findByHandle(huart))
    {
        uart->handleTxComplete();
    }
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    if (auto* uart = platform::stm32f103::Stm32Uart::findByHandle(huart))
    {
        uart->handleRxComplete();
    }
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    if (auto* uart = platform::stm32f103::Stm32Uart::findByHandle(huart))
    {
        uart->handleError();
    }
}

extern "C" void HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef* huart)
{
    if (auto* uart = platform::stm32f103::Stm32Uart::findByHandle(huart))
    {
        uart->handleAbortTransmitComplete();
    }
}

extern "C" void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef* huart)
{
    if (auto* uart = platform::stm32f103::Stm32Uart::findByHandle(huart))
    {
        uart->handleAbortReceiveComplete();
    }
}
