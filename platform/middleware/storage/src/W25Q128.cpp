#include "platform/middleware/storage/W25Q128.hpp"

#include <algorithm>
#include <array>

namespace
{

constexpr std::uint8_t kReadStatusRegister1 = 0x05U;
constexpr std::uint8_t kWriteEnable = 0x06U;
constexpr std::uint8_t kPageProgram = 0x02U;
constexpr std::uint8_t kReadData = 0x03U;
constexpr std::uint8_t kSectorErase4K = 0x20U;
constexpr std::uint8_t kReadJedecId = 0x9FU;

constexpr std::uint8_t kWriteInProgressMask = 0x01U;
constexpr std::uint32_t kSpiTimeoutMs = 100U;

std::array<std::uint8_t, 4U> makeAddressCommand(std::uint8_t command, std::uint32_t address)
{
    return {command, static_cast<std::uint8_t>(address >> 16U),
            static_cast<std::uint8_t>(address >> 8U), static_cast<std::uint8_t>(address)};
}

} // namespace

namespace platform::middleware::storage
{

W25Q128::W25Q128(platform::hal::ISpi& spi, platform::hal::IGpio& chipSelect,
                 std::size_t busyPollLimit)
    : spi_(spi)
    , chipSelect_(chipSelect)
    , busyPollLimit_(busyPollLimit)
{
}

platform::Result<void> W25Q128::initialize()
{
    initialized_ = false;

    if (busyPollLimit_ == 0U)
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    const auto idleResult = chipSelect_.write(platform::hal::GpioState::High);
    if (!idleResult)
    {
        return idleResult;
    }

    std::uint8_t manufacturer = 0U;
    std::uint8_t memoryType = 0U;
    std::uint8_t capacity = 0U;
    const auto idResult = readJedecId(manufacturer, memoryType, capacity);
    if (!idResult)
    {
        return idResult;
    }

    if (manufacturer != 0xEFU || memoryType != 0x40U || capacity != 0x18U)
    {
        return platform::Result<void>::failure(platform::ErrorCode::NotFound);
    }

    initialized_ = true;
    return platform::Result<void>::success();
}

platform::Result<platform::hal::FlashInfo> W25Q128::getInfo() const
{
    if (!initialized_)
    {
        return platform::Result<platform::hal::FlashInfo>::failure(platform::ErrorCode::NotReady);
    }

    const platform::hal::FlashInfo info{0U, kCapacityBytes, kSectorSizeBytes, 1U};
    return platform::Result<platform::hal::FlashInfo>::success(info);
}

platform::Result<void> W25Q128::erase(std::uint32_t address, std::size_t size)
{
    if (!initialized_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::NotReady);
    }

    if (!isValidRange(address, size) || (address % kSectorSizeBytes) != 0U ||
        (size % kSectorSizeBytes) != 0U)
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    const auto initialReadyResult = waitUntilReady();
    if (!initialReadyResult)
    {
        return initialReadyResult;
    }

    for (std::size_t erased = 0U; erased < size; erased += kSectorSizeBytes)
    {
        const std::uint32_t sectorAddress = address + static_cast<std::uint32_t>(erased);
        const auto eraseResult = eraseSector(sectorAddress);
        if (!eraseResult)
        {
            return eraseResult;
        }

        const auto completionResult = waitUntilReady();
        if (!completionResult)
        {
            return completionResult;
        }
    }

    return platform::Result<void>::success();
}

platform::Result<void> W25Q128::write(std::uint32_t address, const std::uint8_t* data,
                                      std::size_t size)
{
    if (!initialized_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::NotReady);
    }

    if (!isValidRange(address, size) || (size > 0U && data == nullptr))
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    if (size == 0U)
    {
        return platform::Result<void>::success();
    }

    const auto initialReadyResult = waitUntilReady();
    if (!initialReadyResult)
    {
        return initialReadyResult;
    }

    std::size_t written = 0U;
    while (written < size)
    {
        const std::uint32_t currentAddress = address + static_cast<std::uint32_t>(written);
        const std::size_t pageOffset = currentAddress % kPageSizeBytes;
        const std::size_t pageRemaining = kPageSizeBytes - pageOffset;
        const std::size_t chunkSize = std::min(size - written, pageRemaining);

        const auto programResult = programPage(currentAddress, data + written, chunkSize);
        if (!programResult)
        {
            return programResult;
        }

        const auto completionResult = waitUntilReady();
        if (!completionResult)
        {
            return completionResult;
        }

        written += chunkSize;
    }

    return platform::Result<void>::success();
}

platform::Result<void> W25Q128::read(std::uint32_t address, std::uint8_t* data, std::size_t size)
{
    if (!initialized_)
    {
        return platform::Result<void>::failure(platform::ErrorCode::NotReady);
    }

    if (!isValidRange(address, size) || (size > 0U && data == nullptr))
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    if (size == 0U)
    {
        return platform::Result<void>::success();
    }

    const auto readyResult = waitUntilReady();
    if (!readyResult)
    {
        return readyResult;
    }

    const auto selectResult = chipSelect_.write(platform::hal::GpioState::Low);
    if (!selectResult)
    {
        return selectResult;
    }

    const auto command = makeAddressCommand(kReadData, address);
    auto transmitResult = spi_.transmit(command.data(), command.size(), kSpiTimeoutMs);
    platform::ErrorCode operationError =
        transmitResult ? platform::ErrorCode::Ok : transmitResult.error();

    std::size_t received = 0U;
    while (operationError == platform::ErrorCode::Ok && received < size)
    {
        const std::size_t chunkSize = std::min(size - received, static_cast<std::size_t>(0xFFFFU));
        const auto receiveResult = spi_.receive(data + received, chunkSize, kSpiTimeoutMs);
        if (!receiveResult)
        {
            operationError = receiveResult.error();
        }
        else
        {
            received += chunkSize;
        }
    }

    return finishTransaction(operationError);
}

bool W25Q128::isValidRange(std::uint32_t address, std::size_t size)
{
    return address <= kCapacityBytes && size <= static_cast<std::size_t>(kCapacityBytes - address);
}

platform::Result<void> W25Q128::readJedecId(std::uint8_t& manufacturer, std::uint8_t& memoryType,
                                            std::uint8_t& capacity)
{
    const auto selectResult = chipSelect_.write(platform::hal::GpioState::Low);
    if (!selectResult)
    {
        return selectResult;
    }

    const std::uint8_t command = kReadJedecId;
    auto transmitResult = spi_.transmit(&command, 1U, kSpiTimeoutMs);
    platform::ErrorCode operationError =
        transmitResult ? platform::ErrorCode::Ok : transmitResult.error();

    std::array<std::uint8_t, 3U> response{};
    if (operationError == platform::ErrorCode::Ok)
    {
        const auto receiveResult = spi_.receive(response.data(), response.size(), kSpiTimeoutMs);
        if (!receiveResult)
        {
            operationError = receiveResult.error();
        }
    }

    const auto finishResult = finishTransaction(operationError);
    if (!finishResult)
    {
        return finishResult;
    }

    manufacturer = response[0];
    memoryType = response[1];
    capacity = response[2];
    return platform::Result<void>::success();
}

platform::Result<void> W25Q128::readStatus(std::uint8_t& status)
{
    const auto selectResult = chipSelect_.write(platform::hal::GpioState::Low);
    if (!selectResult)
    {
        return selectResult;
    }

    const std::uint8_t command = kReadStatusRegister1;
    auto transmitResult = spi_.transmit(&command, 1U, kSpiTimeoutMs);
    platform::ErrorCode operationError =
        transmitResult ? platform::ErrorCode::Ok : transmitResult.error();

    std::uint8_t response = 0U;
    if (operationError == platform::ErrorCode::Ok)
    {
        const auto receiveResult = spi_.receive(&response, 1U, kSpiTimeoutMs);
        if (!receiveResult)
        {
            operationError = receiveResult.error();
        }
    }

    const auto finishResult = finishTransaction(operationError);
    if (!finishResult)
    {
        return finishResult;
    }

    status = response;
    return platform::Result<void>::success();
}

platform::Result<void> W25Q128::waitUntilReady()
{
    for (std::size_t poll = 0U; poll < busyPollLimit_; ++poll)
    {
        std::uint8_t status = 0U;
        const auto statusResult = readStatus(status);
        if (!statusResult)
        {
            return statusResult;
        }

        if ((status & kWriteInProgressMask) == 0U)
        {
            return platform::Result<void>::success();
        }
    }

    return platform::Result<void>::failure(platform::ErrorCode::Timeout);
}

platform::Result<void> W25Q128::writeEnable()
{
    const auto selectResult = chipSelect_.write(platform::hal::GpioState::Low);
    if (!selectResult)
    {
        return selectResult;
    }

    const std::uint8_t command = kWriteEnable;
    const auto transmitResult = spi_.transmit(&command, 1U, kSpiTimeoutMs);
    const platform::ErrorCode operationError =
        transmitResult ? platform::ErrorCode::Ok : transmitResult.error();
    return finishTransaction(operationError);
}

platform::Result<void> W25Q128::finishTransaction(platform::ErrorCode operationError)
{
    const auto deselectResult = chipSelect_.write(platform::hal::GpioState::High);
    if (operationError != platform::ErrorCode::Ok)
    {
        return platform::Result<void>::failure(operationError);
    }

    if (!deselectResult)
    {
        return platform::Result<void>::failure(deselectResult.error());
    }

    return platform::Result<void>::success();
}

platform::Result<void> W25Q128::programPage(std::uint32_t address, const std::uint8_t* data,
                                            std::size_t size)
{
    const auto enableResult = writeEnable();
    if (!enableResult)
    {
        return enableResult;
    }

    const auto selectResult = chipSelect_.write(platform::hal::GpioState::Low);
    if (!selectResult)
    {
        return selectResult;
    }

    const auto command = makeAddressCommand(kPageProgram, address);
    auto transmitResult = spi_.transmit(command.data(), command.size(), kSpiTimeoutMs);
    platform::ErrorCode operationError =
        transmitResult ? platform::ErrorCode::Ok : transmitResult.error();

    if (operationError == platform::ErrorCode::Ok)
    {
        transmitResult = spi_.transmit(data, size, kSpiTimeoutMs);
        if (!transmitResult)
        {
            operationError = transmitResult.error();
        }
    }

    return finishTransaction(operationError);
}

platform::Result<void> W25Q128::eraseSector(std::uint32_t address)
{
    const auto enableResult = writeEnable();
    if (!enableResult)
    {
        return enableResult;
    }

    const auto selectResult = chipSelect_.write(platform::hal::GpioState::Low);
    if (!selectResult)
    {
        return selectResult;
    }

    const auto command = makeAddressCommand(kSectorErase4K, address);
    const auto transmitResult = spi_.transmit(command.data(), command.size(), kSpiTimeoutMs);
    const platform::ErrorCode operationError =
        transmitResult ? platform::ErrorCode::Ok : transmitResult.error();
    return finishTransaction(operationError);
}

} // namespace platform::middleware::storage