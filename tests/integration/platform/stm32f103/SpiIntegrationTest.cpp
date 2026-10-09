#include <array>
#include <cstdint>

#include "platform/bsp/stm32f103_board/Board.hpp"
#include "platform/hal/IGpio.hpp"
#include "platform/hal/ISpi.hpp"

#include "stm32f1xx.h"

namespace
{

enum class TestStatus : std::uint32_t
{
    NotStarted = 0x0000,
    SpiConfigured = 0x0001,
    ChipSelected = 0x0002,
    JedecIdRead = 0x0003,
    Passed = 0xAA55,
    SpiConfigureFailed = 0x1001,
    ChipSelectFailed = 0x1002,
    TransferFailed = 0x1003,
    ChipDeselectFailed = 0x1004,
    JedecIdMismatch = 0x1005
};

volatile TestStatus g_spi_test_status = TestStatus::NotStarted;
volatile std::uint32_t g_spi_jedec_id = 0U;

[[noreturn]] void testFailure(TestStatus status)
{
    g_spi_test_status = status;
    __disable_irq();

    while (true)
    {
        __asm volatile("bkpt #0");
    }
}

} // namespace

int main()
{
    platform::bsp::stm32f103_board::init();

    platform::hal::ISpi& spi = platform::bsp::stm32f103_board::spi1();
    platform::hal::IGpio& chipSelect = platform::bsp::stm32f103_board::spi1ChipSelect();

    platform::hal::SpiConfig config{};
    config.frequencyHz = 1'000'000U;
    config.mode = platform::hal::SpiMode::Mode0;
    config.bitOrder = platform::hal::SpiBitOrder::MsbFirst;
    config.dataBits = 8U;

    if (!spi.configure(config))
    {
        testFailure(TestStatus::SpiConfigureFailed);
    }

    g_spi_test_status = TestStatus::SpiConfigured;

    constexpr std::array<std::uint8_t, 4U> kTransmit{0x9FU, 0xFFU, 0xFFU, 0xFFU};
    std::array<std::uint8_t, kTransmit.size()> receive{};

    if (!chipSelect.write(platform::hal::GpioState::Low))
    {
        testFailure(TestStatus::ChipSelectFailed);
    }

    g_spi_test_status = TestStatus::ChipSelected;
    const auto transferResult =
        spi.transfer(kTransmit.data(), receive.data(), receive.size(), 100U);
    const auto deselectResult = chipSelect.write(platform::hal::GpioState::High);

    if (!deselectResult)
    {
        testFailure(TestStatus::ChipDeselectFailed);
    }

    if (!transferResult)
    {
        testFailure(TestStatus::TransferFailed);
    }

    const std::uint32_t jedecId = (static_cast<std::uint32_t>(receive[1]) << 16U) |
                                  (static_cast<std::uint32_t>(receive[2]) << 8U) |
                                  static_cast<std::uint32_t>(receive[3]);
    g_spi_jedec_id = jedecId;
    g_spi_test_status = TestStatus::JedecIdRead;

    if (jedecId != 0xEF4018U)
    {
        testFailure(TestStatus::JedecIdMismatch);
    }

    g_spi_test_status = TestStatus::Passed;
    __disable_irq();

    while (true)
    {
        __asm volatile("nop");
    }
}