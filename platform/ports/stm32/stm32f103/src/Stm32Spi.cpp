#include "platform/stm32f103/Stm32Spi.hpp"

#include <limits>

namespace platform::stm32f103
{

Stm32Spi::Stm32Spi(SPI_TypeDef* instance)
    : instance_(instance)
{
    handle_.Instance = instance_;
}

platform::Result<void> Stm32Spi::configure(const platform::hal::SpiConfig& config)
{
    if (!isValidInstance(instance_) || !isValidConfig(config))
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    std::uint32_t polarity = 0U;
    std::uint32_t phase = 0U;
    std::uint32_t firstBit = 0U;

    if (!toHalMode(config.mode, polarity, phase) || !toHalBitOrder(config.bitOrder, firstBit))
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    const std::uint32_t peripheralClockHz =
        instance_ == SPI1 ? HAL_RCC_GetPCLK2Freq() : HAL_RCC_GetPCLK1Freq();

    std::uint32_t prescaler = 0U;
    if (!toHalPrescaler(peripheralClockHz, config.frequencyHz, prescaler))
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    configured_ = false;
    handle_.Instance = instance_;
    handle_.Init.Mode = SPI_MODE_MASTER;
    handle_.Init.Direction = SPI_DIRECTION_2LINES;
    handle_.Init.DataSize = SPI_DATASIZE_8BIT;
    handle_.Init.CLKPolarity = polarity;
    handle_.Init.CLKPhase = phase;
    handle_.Init.NSS = SPI_NSS_SOFT;
    handle_.Init.BaudRatePrescaler = prescaler;
    handle_.Init.FirstBit = firstBit;
    handle_.Init.TIMode = SPI_TIMODE_DISABLE;
    handle_.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    handle_.Init.CRCPolynomial = 7U;

    const HAL_StatusTypeDef status = HAL_SPI_Init(&handle_);
    if (status != HAL_OK)
    {
        return platform::Result<void>::failure(mapHalStatus(status));
    }

    configured_ = true;
    return platform::Result<void>::success();
}

platform::Result<void> Stm32Spi::transmit(const std::uint8_t* data, std::size_t size,
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

    const HAL_StatusTypeDef status = HAL_SPI_Transmit(&handle_, const_cast<std::uint8_t*>(data),
                                                      static_cast<std::uint16_t>(size), timeoutMs);
    if (status != HAL_OK)
    {
        return platform::Result<void>::failure(mapHalStatus(status));
    }

    return platform::Result<void>::success();
}

platform::Result<void> Stm32Spi::receive(std::uint8_t* data, std::size_t size,
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
        HAL_SPI_Receive(&handle_, data, static_cast<std::uint16_t>(size), timeoutMs);
    if (status != HAL_OK)
    {
        return platform::Result<void>::failure(mapHalStatus(status));
    }

    return platform::Result<void>::success();
}

platform::Result<void> Stm32Spi::transfer(const std::uint8_t* txData, std::uint8_t* rxData,
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

    if (txData == nullptr || rxData == nullptr || size > std::numeric_limits<std::uint16_t>::max())
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    const HAL_StatusTypeDef status =
        HAL_SPI_TransmitReceive(&handle_, const_cast<std::uint8_t*>(txData), rxData,
                                static_cast<std::uint16_t>(size), timeoutMs);
    if (status != HAL_OK)
    {
        return platform::Result<void>::failure(mapHalStatus(status));
    }

    return platform::Result<void>::success();
}

bool Stm32Spi::isValidInstance(SPI_TypeDef* instance)
{
    return instance == SPI1 || instance == SPI2;
}

bool Stm32Spi::isValidConfig(const platform::hal::SpiConfig& config)
{
    return config.frequencyHz > 0U && config.dataBits == 8U;
}

bool Stm32Spi::toHalMode(platform::hal::SpiMode mode, std::uint32_t& polarity, std::uint32_t& phase)
{
    switch (mode)
    {
    case platform::hal::SpiMode::Mode0:
        polarity = SPI_POLARITY_LOW;
        phase = SPI_PHASE_1EDGE;
        return true;

    case platform::hal::SpiMode::Mode1:
        polarity = SPI_POLARITY_LOW;
        phase = SPI_PHASE_2EDGE;
        return true;

    case platform::hal::SpiMode::Mode2:
        polarity = SPI_POLARITY_HIGH;
        phase = SPI_PHASE_1EDGE;
        return true;

    case platform::hal::SpiMode::Mode3:
        polarity = SPI_POLARITY_HIGH;
        phase = SPI_PHASE_2EDGE;
        return true;

    default:
        return false;
    }
}

bool Stm32Spi::toHalBitOrder(platform::hal::SpiBitOrder bitOrder, std::uint32_t& firstBit)
{
    switch (bitOrder)
    {
    case platform::hal::SpiBitOrder::MsbFirst:
        firstBit = SPI_FIRSTBIT_MSB;
        return true;

    case platform::hal::SpiBitOrder::LsbFirst:
        firstBit = SPI_FIRSTBIT_LSB;
        return true;

    default:
        return false;
    }
}

bool Stm32Spi::toHalPrescaler(std::uint32_t peripheralClockHz, std::uint32_t requestedHz,
                              std::uint32_t& prescaler)
{
    constexpr std::uint32_t kDivisors[] = {2U, 4U, 8U, 16U, 32U, 64U, 128U, 256U};
    constexpr std::uint32_t kPrescalers[] = {SPI_BAUDRATEPRESCALER_2,   SPI_BAUDRATEPRESCALER_4,
                                             SPI_BAUDRATEPRESCALER_8,   SPI_BAUDRATEPRESCALER_16,
                                             SPI_BAUDRATEPRESCALER_32,  SPI_BAUDRATEPRESCALER_64,
                                             SPI_BAUDRATEPRESCALER_128, SPI_BAUDRATEPRESCALER_256};

    if (peripheralClockHz == 0U || requestedHz == 0U)
    {
        return false;
    }

    for (std::size_t index = 0U; index < (sizeof(kDivisors) / sizeof(kDivisors[0])); ++index)
    {
        if ((peripheralClockHz / kDivisors[index]) <= requestedHz)
        {
            prescaler = kPrescalers[index];
            return true;
        }
    }

    return false;
}

platform::ErrorCode Stm32Spi::mapHalStatus(HAL_StatusTypeDef status)
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