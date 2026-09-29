#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/common/NonCopyable.hpp"
#include "platform/hal/IAsyncUart.hpp"
#include "platform/hal/IUart.hpp"

#include "stm32f1xx_hal.h"

namespace platform::stm32f103
{

class Stm32Uart final : public platform::hal::IUart,
                        public platform::hal::IAsyncUart,
                        public platform::NonCopyable
{
public:
    explicit Stm32Uart(USART_TypeDef* instance);

    ~Stm32Uart() override;

    platform::Result<void> configure(const platform::hal::UartConfig& config) override;

    platform::Result<void> transmit(const std::uint8_t* data, std::size_t size,
                                    std::uint32_t timeoutMs) override;

    platform::Result<std::size_t> receive(std::uint8_t* data, std::size_t size,
                                          std::uint32_t timeoutMs) override;

    platform::Result<void> transmitAsync(const std::uint8_t* data, std::size_t size,
                                         platform::hal::UartTransferCallback callback,
                                         void* context) override;

    platform::Result<void> receiveAsync(std::uint8_t* data, std::size_t size,
                                        platform::hal::UartTransferCallback callback,
                                        void* context) override;

    platform::Result<void> cancelTransmit() override;

    platform::Result<void> cancelReceive() override;

    void handleTxComplete();
    void handleRxComplete();
    void handleError();
    void handleAbortTransmitComplete();
    void handleAbortReceiveComplete();

    static Stm32Uart* findByHandle(UART_HandleTypeDef* handle);
    static void irqHandler(USART_TypeDef* instance);

private:
    static bool isValidInstance(USART_TypeDef* instance);
    static bool isValidConfig(const platform::hal::UartConfig& config);

    static std::uint32_t toHalWordLength(const platform::hal::UartConfig& config);
    static std::uint32_t toHalParity(platform::hal::UartParity parity);
    static std::uint32_t toHalStopBits(platform::hal::UartStopBits stopBits);
    static std::uint32_t toHalFlowControl(platform::hal::UartFlowControl flowControl);

    static platform::ErrorCode mapHalStatus(HAL_StatusTypeDef status);

    static Stm32Uart* findByInstance(USART_TypeDef* instance);
    bool registerInstance();
    void unregisterInstance();

    static std::size_t transferredBytes(std::uint16_t requested, std::uint16_t remaining);

    void clearTransmitCallbackState();
    void clearReceiveCallbackState();

private:
    USART_TypeDef* instance_;
    UART_HandleTypeDef handle_{};

    bool configured_{false};
    bool registered_{false};

    volatile bool txActive_{false};
    volatile bool rxActive_{false};

    volatile std::uint16_t txRequestedSize_{0U};
    volatile std::uint16_t rxRequestedSize_{0U};

    volatile std::size_t txAbortTransferred_{0U};
    volatile std::size_t rxAbortTransferred_{0U};

    platform::hal::UartTransferCallback volatile txCallback_{nullptr};
    void* volatile txContext_{nullptr};

    platform::hal::UartTransferCallback volatile rxCallback_{nullptr};
    void* volatile rxContext_{nullptr};
};

} // namespace platform::stm32f103
