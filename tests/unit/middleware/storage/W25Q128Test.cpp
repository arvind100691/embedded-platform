#include "platform/middleware/storage/W25Q128.hpp"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

#include "MockGpio.hpp"
#include "MockSpi.hpp"

namespace
{

void configureSpi(MockSpi& spi)
{
    platform::hal::SpiConfig config{};
    EXPECT_TRUE(spi.configure(config));
}

void enqueueW25Q128Id(MockSpi& spi)
{
    spi.enqueueReceivedData({0xEFU, 0x40U, 0x18U});
}

bool initializeFlash(MockSpi& spi, MockGpio& chipSelect,
                     platform::middleware::storage::W25Q128& flash)
{
    configureSpi(spi);
    enqueueW25Q128Id(spi);
    return static_cast<bool>(flash.initialize());
}

} // namespace

TEST(W25Q128Test, InitializeValidatesJedecIdAndReportsGeometry)
{
    MockSpi spi;
    MockGpio chipSelect;
    platform::middleware::storage::W25Q128 flash(spi, chipSelect);
    configureSpi(spi);
    enqueueW25Q128Id(spi);

    const auto initializeResult = flash.initialize();
    ASSERT_TRUE(initializeResult);
    EXPECT_EQ(chipSelect.state(), platform::hal::GpioState::High);

    const auto infoResult = flash.getInfo();
    ASSERT_TRUE(infoResult);
    ASSERT_NE(infoResult.value(), nullptr);
    EXPECT_EQ(infoResult.value()->size, 16U * 1024U * 1024U);
    EXPECT_EQ(infoResult.value()->eraseBlockSize, 4U * 1024U);
    EXPECT_EQ(infoResult.value()->programUnitSize, 1U);
}

TEST(W25Q128Test, InitializeRejectsDifferentDevice)
{
    MockSpi spi;
    MockGpio chipSelect;
    platform::middleware::storage::W25Q128 flash(spi, chipSelect);
    configureSpi(spi);
    spi.enqueueReceivedData({0xEFU, 0x40U, 0x17U});

    const auto result = flash.initialize();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::NotFound);
    EXPECT_FALSE(flash.getInfo());
}

TEST(W25Q128Test, ReadSendsAddressAndReturnsData)
{
    MockSpi spi;
    MockGpio chipSelect;
    platform::middleware::storage::W25Q128 flash(spi, chipSelect);
    ASSERT_TRUE(initializeFlash(spi, chipSelect, flash));
    spi.enqueueReceivedData({0x00U, 0xA5U, 0x5AU});

    std::array<std::uint8_t, 2U> data{};
    const auto result = flash.read(0x000123U, data.data(), data.size());

    ASSERT_TRUE(result);
    EXPECT_EQ(data[0], 0xA5U);
    EXPECT_EQ(data[1], 0x5AU);
    EXPECT_EQ(chipSelect.state(), platform::hal::GpioState::High);
    EXPECT_EQ(spi.transmittedData(),
              (std::vector<std::uint8_t>{0x9FU, 0x05U, 0x03U, 0x00U, 0x01U, 0x23U}));
}

TEST(W25Q128Test, WriteSplitsPayloadAtPageBoundary)
{
    MockSpi spi;
    MockGpio chipSelect;
    platform::middleware::storage::W25Q128 flash(spi, chipSelect);
    ASSERT_TRUE(initializeFlash(spi, chipSelect, flash));
    spi.enqueueReceivedData({0x00U, 0x00U, 0x00U, 0x00U});

    constexpr std::array<std::uint8_t, 4U> data{0xA1U, 0xB2U, 0xC3U, 0xD4U};
    const auto result = flash.write(0x0000FEU, data.data(), data.size());

    ASSERT_TRUE(result);
    EXPECT_EQ(chipSelect.state(), platform::hal::GpioState::High);
    EXPECT_EQ(
        spi.transmittedData(),
        (std::vector<std::uint8_t>{0x9FU, 0x05U, 0x06U, 0x02U, 0x00U, 0x00U, 0xFEU, 0xA1U, 0xB2U,
                                   0x05U, 0x06U, 0x02U, 0x00U, 0x01U, 0x00U, 0xC3U, 0xD4U, 0x05U}));
}

TEST(W25Q128Test, EraseRejectsUnalignedRange)
{
    MockSpi spi;
    MockGpio chipSelect;
    platform::middleware::storage::W25Q128 flash(spi, chipSelect);
    ASSERT_TRUE(initializeFlash(spi, chipSelect, flash));

    const auto result = flash.erase(1U, platform::middleware::storage::W25Q128::kSectorSizeBytes);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::InvalidArgument);
}

TEST(W25Q128Test, EraseIssuesSectorCommandAndWaitsForCompletion)
{
    MockSpi spi;
    MockGpio chipSelect;
    platform::middleware::storage::W25Q128 flash(spi, chipSelect);
    ASSERT_TRUE(initializeFlash(spi, chipSelect, flash));
    spi.enqueueReceivedData({0x00U, 0x00U});

    const auto result =
        flash.erase(0x001000U, platform::middleware::storage::W25Q128::kSectorSizeBytes);

    ASSERT_TRUE(result);
    EXPECT_EQ(chipSelect.state(), platform::hal::GpioState::High);
    EXPECT_EQ(spi.transmittedData(),
              (std::vector<std::uint8_t>{0x9FU, 0x05U, 0x06U, 0x20U, 0x00U, 0x10U, 0x00U, 0x05U}));
}

TEST(W25Q128Test, ZeroLengthWriteDoesNotAccessTheBus)
{
    MockSpi spi;
    MockGpio chipSelect;
    platform::middleware::storage::W25Q128 flash(spi, chipSelect);
    ASSERT_TRUE(initializeFlash(spi, chipSelect, flash));
    const auto transmittedBefore = spi.transmittedData();

    const auto result = flash.write(0U, nullptr, 0U);

    ASSERT_TRUE(result);
    EXPECT_EQ(spi.transmittedData(), transmittedBefore);
}

TEST(W25Q128Test, BusyPollingReturnsTimeoutAtConfiguredLimit)
{
    MockSpi spi;
    MockGpio chipSelect;
    platform::middleware::storage::W25Q128 flash(spi, chipSelect, 2U);
    ASSERT_TRUE(initializeFlash(spi, chipSelect, flash));
    spi.enqueueReceivedData({0x01U, 0x01U});

    std::uint8_t data = 0U;
    const auto result = flash.read(0U, &data, 1U);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::Timeout);
    EXPECT_EQ(chipSelect.state(), platform::hal::GpioState::High);
}

TEST(W25Q128Test, OperationsRequireInitialization)
{
    MockSpi spi;
    MockGpio chipSelect;
    platform::middleware::storage::W25Q128 flash(spi, chipSelect);
    std::uint8_t data = 0U;

    const auto result = flash.read(0U, &data, 1U);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::NotReady);
}

TEST(W25Q128Test, RejectsRangesPastDeviceCapacity)
{
    MockSpi spi;
    MockGpio chipSelect;
    platform::middleware::storage::W25Q128 flash(spi, chipSelect);
    ASSERT_TRUE(initializeFlash(spi, chipSelect, flash));
    std::uint8_t data = 0U;

    const auto result =
        flash.read(platform::middleware::storage::W25Q128::kCapacityBytes, &data, 1U);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::InvalidArgument);
}