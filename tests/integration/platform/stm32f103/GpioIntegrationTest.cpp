#include <cstdint>

#include "platform/hal/IGpio.hpp"
#include "platform/stm32f103/Stm32Gpio.hpp"
#include "platform/bsp/stm32f103_board/Board.hpp"

namespace
{

enum class TestStatus : std::uint32_t
{
    NotStarted                = 0x0000,

    OutputConfigured          = 0x0001,
    OutputLowPassed           = 0x0002,
    OutputHighPassed          = 0x0003,

    InputConfigured           = 0x0004,
    CallbackRegistered        = 0x0005,
    InterruptEnabled          = 0x0006,

    OutputDrivenLow           = 0x0007,
    OutputDrivenHigh          = 0x0008,

    InterruptPassed           = 0x0009,

    Passed                    = 0xAA55,

    OutputConfigureFailed     = 0x1001,
    OutputWriteLowFailed      = 0x1002,
    OutputReadLowFailed       = 0x1003,
    OutputLowCheckFailed      = 0x1004,

    OutputWriteHighFailed     = 0x1005,
    OutputReadHighFailed      = 0x1006,
    OutputHighCheckFailed     = 0x1007,

    InputConfigureFailed      = 0x2001,
    CallbackRegisterFailed    = 0x2002,
    InterruptEnableFailed     = 0x2003,

    InterruptTimeout          = 0x3001
};

volatile TestStatus g_gpio_test_status = TestStatus::NotStarted;
volatile std::uint32_t g_gpio_interrupt_count = 0U;

/*
 * GPIO interrupt callback.
 *
 * This callback is expected to execute in interrupt context.
 * Keep it short and non-blocking.
 */
void gpioInterruptCallback(void* /*context*/)
{
    ++g_gpio_interrupt_count;
}

/*
 * Stop the test at a deterministic failure point.
 *
 * The debugger can inspect:
 *
 *     g_gpio_test_status
 *     g_gpio_interrupt_count
 */
[[noreturn]] void testFailure(TestStatus status)
{
    g_gpio_test_status = status;

    __disable_irq();

    while (true)
    {
        __asm volatile("bkpt #0");
    }
}

/*
 * Wait for the interrupt callback without an infinite wait.
 */
bool waitForInterrupt()
{
    constexpr std::uint32_t kTimeoutIterations = 1'000'000U;

    std::uint32_t timeout = kTimeoutIterations;

    while ((g_gpio_interrupt_count == 0U) && (timeout > 0U))
    {
        --timeout;
        __asm volatile("nop");
    }

    return g_gpio_interrupt_count > 0U;
}

} // namespace

int main()
{
    /*
     * ================================================================
     * BOARD INITIALIZATION
     * ================================================================
     *
     * The BSP exposes init() as a namespace function.
     */
    platform::bsp::stm32f103_board::init();

    /*
     * ================================================================
     * 1. PC13 OUTPUT
     * ================================================================
     */
    platform::stm32f103::Stm32Gpio output(GPIOC, GPIO_PIN_13);

    platform::hal::GpioConfig outputConfig{};
    outputConfig.direction =
        platform::hal::GpioDirection::Output;

    outputConfig.pull =
        platform::hal::GpioPull::None;

    outputConfig.initialState =
        platform::hal::GpioState::Low;

    outputConfig.interruptEdge =
        platform::hal::GpioInterruptEdge::None;

    if (!output.configure(outputConfig))
    {
        testFailure(TestStatus::OutputConfigureFailed);
    }

    g_gpio_test_status = TestStatus::OutputConfigured;

    /*
     * ================================================================
     * 2. PC13 LOW
     * ================================================================
     */
    if (!output.write(platform::hal::GpioState::Low))
    {
        testFailure(TestStatus::OutputWriteLowFailed);
    }

    /*
     * read() returns:
     *
     *     Result<GpioState>
     */
    const auto lowResult = output.read();

    if (!lowResult)
    {
        testFailure(TestStatus::OutputReadLowFailed);
    }

    if (*lowResult.value() != platform::hal::GpioState::Low)
    {
        testFailure(TestStatus::OutputLowCheckFailed);
    }

    g_gpio_test_status = TestStatus::OutputLowPassed;

    /*
     * ================================================================
     * 3. PC13 HIGH
     * ================================================================
     */
    if (!output.write(platform::hal::GpioState::High))
    {
        testFailure(TestStatus::OutputWriteHighFailed);
    }

    const auto highResult = output.read();

    if (!highResult)
    {
        testFailure(TestStatus::OutputReadHighFailed);
    }

    if (*highResult.value() != platform::hal::GpioState::High)
    {
        testFailure(TestStatus::OutputHighCheckFailed);
    }

    g_gpio_test_status = TestStatus::OutputHighPassed;

    /*
     * ================================================================
     * 4. PA0 INPUT + RISING EDGE
     * ================================================================
     *
     * Hardware connection:
     *
     *     PC13 ---------------- PA0
     */
    platform::stm32f103::Stm32Gpio input(GPIOA, GPIO_PIN_0);

    platform::hal::GpioConfig inputConfig{};
    inputConfig.direction =
        platform::hal::GpioDirection::Input;

    inputConfig.pull =
        platform::hal::GpioPull::PullDown;

    inputConfig.initialState =
        platform::hal::GpioState::Low;

    inputConfig.interruptEdge =
        platform::hal::GpioInterruptEdge::Rising;

    if (!input.configure(inputConfig))
    {
        testFailure(TestStatus::InputConfigureFailed);
    }

    g_gpio_test_status = TestStatus::InputConfigured;

    /*
     * ================================================================
     * 5. REGISTER INTERRUPT CALLBACK
     * ================================================================
     */
    if (!input.registerInterruptCallback(
            gpioInterruptCallback,
            nullptr))
    {
        testFailure(TestStatus::CallbackRegisterFailed);
    }

    g_gpio_test_status = TestStatus::CallbackRegistered;

    /*
     * ================================================================
     * 6. ENABLE INTERRUPT
     * ================================================================
     */
    if (!input.enableInterrupt())
    {
        testFailure(TestStatus::InterruptEnableFailed);
    }

    g_gpio_test_status = TestStatus::InterruptEnabled;

    /*
     * ================================================================
     * 7. FORCE PA0 LOW
     * ================================================================
     */
    g_gpio_interrupt_count = 0U;

    if (!output.write(platform::hal::GpioState::Low))
    {
        testFailure(TestStatus::OutputWriteLowFailed);
    }

    g_gpio_test_status = TestStatus::OutputDrivenLow;

    /*
     * Allow the signal to settle.
     */
    for (volatile std::uint32_t i = 0U;
         i < 1000U;
         ++i)
    {
        __asm volatile("nop");
    }

    /*
     * ================================================================
     * 8. GENERATE RISING EDGE
     * ================================================================
     *
     * PC13:
     *
     *     LOW  -> HIGH
     *
     * Because PC13 is connected to PA0:
     *
     *     PA0:
     *
     *     LOW  -> HIGH
     *
     * This should trigger EXTI0.
     */
    if (!output.write(platform::hal::GpioState::High))
    {
        testFailure(TestStatus::OutputWriteHighFailed);
    }

    g_gpio_test_status = TestStatus::OutputDrivenHigh;

    /*
     * ================================================================
     * 9. WAIT FOR INTERRUPT
     * ================================================================
     */
    if (!waitForInterrupt())
    {
        testFailure(TestStatus::InterruptTimeout);
    }

    g_gpio_test_status = TestStatus::InterruptPassed;

    /*
     * ================================================================
     * 10. PASS
     * ================================================================
     */
    g_gpio_test_status = TestStatus::Passed;

    /*
     * Keep the MCU here so the debugger can inspect the final result.
     */
    __disable_irq();

    while (true)
    {
        __asm volatile("nop");
    }
}