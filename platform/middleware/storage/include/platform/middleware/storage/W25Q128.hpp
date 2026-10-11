#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/common/NonCopyable.hpp"
#include "platform/hal/IFlashStorage.hpp"
#include "platform/hal/IGpio.hpp"
#include "platform/hal/ISpi.hpp"

namespace platform::middleware::storage
{

class W25Q128 final : public platform::hal::IFlashStorage, public platform::NonCopyable
{
public:
    static constexpr std::uint32_t kCapacityBytes = 16U * 1024U * 1024U;
    static constexpr std::uint32_t kSectorSizeBytes = 4U * 1024U;
    static constexpr std::size_t kPageSizeBytes = 256U;

    explicit W25Q128(platform::hal::ISpi& spi, platform::hal::IGpio& chipSelect,
                     std::size_t busyPollLimit = 100'000U);

    platform::Result<void> initialize();

    platform::Result<platform::hal::FlashInfo> getInfo() const override;

    platform::Result<void> erase(std::uint32_t address, std::size_t size) override;

    platform::Result<void> write(std::uint32_t address, const std::uint8_t* data,
                                 std::size_t size) override;

    platform::Result<void> read(std::uint32_t address, std::uint8_t* data,
                                std::size_t size) override;

private:
    static bool isValidRange(std::uint32_t address, std::size_t size);

    platform::Result<void> readJedecId(std::uint8_t& manufacturer, std::uint8_t& memoryType,
                                       std::uint8_t& capacity);
    platform::Result<void> readStatus(std::uint8_t& status);
    platform::Result<void> waitUntilReady();
    platform::Result<void> writeEnable();
    platform::Result<void> finishTransaction(platform::ErrorCode operationError);

    platform::Result<void> programPage(std::uint32_t address, const std::uint8_t* data,
                                       std::size_t size);
    platform::Result<void> eraseSector(std::uint32_t address);

    platform::hal::ISpi& spi_;
    platform::hal::IGpio& chipSelect_;
    std::size_t busyPollLimit_;
    bool initialized_{false};
};

} // namespace platform::middleware::storage