#include "app/Application.hpp"

#include <array>
#include <cstdint>

#include "platform/middleware/cli/CommandRegistry.hpp"
#include "platform/middleware/cli/ConsoleCli.hpp"

namespace
{

constexpr std::uint32_t kDelayIterationsPerCliPoll = 25000U;
constexpr std::uint32_t kCliPollsPerBlinkInterval = 20U;

std::size_t appendUnsigned(char* buffer, std::size_t capacity, std::size_t position,
                           std::uint32_t value)
{
    std::array<char, 10U> digits{};
    std::size_t digitCount{0U};
    do
    {
        digits[digitCount++] = static_cast<char>('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U && digitCount < digits.size());

    while (digitCount > 0U && position < capacity)
    {
        buffer[position++] = digits[--digitCount];
    }
    return position;
}

platform::Result<void> writeSensorError(platform::middleware::cli::IConsole& console,
                                        platform::ErrorCode error)
{
    std::array<char, 64U> message{};
    constexpr char kPrefix[] = "MAX30102 error: ";
    std::size_t size{0U};
    for (const char character : kPrefix)
    {
        if (character == '\0')
        {
            break;
        }
        message[size++] = character;
    }
    size = appendUnsigned(message.data(), message.size(), size,
                          static_cast<std::uint32_t>(error));
    if (error == platform::ErrorCode::Busy)
    {
        constexpr char kBusyExplanation[] = " (I2C bus busy; retrying)";
        for (const char character : kBusyExplanation)
        {
            if (character == '\0')
            {
                break;
            }
            message[size++] = character;
        }
    }
    message[size++] = '\r';
    message[size++] = '\n';
    return console.write(std::string_view(message.data(), size));
}

platform::Result<void> writeRawSample(platform::middleware::cli::IConsole& console,
                                      const platform::drivers::max30102::Max30102Sample& sample,
                                      bool hasHeartRate, std::uint16_t heartRateBpm)
{
    std::array<char, 48U> message{};
    constexpr char kRedPrefix[] = "RED=";
    std::size_t size{0U};
    for (const char character : kRedPrefix)
    {
        if (character == '\0')
        {
            break;
        }
        message[size++] = character;
    }
    size = appendUnsigned(message.data(), message.size(), size, sample.red);
    constexpr char kInfraredPrefix[] = " IR=";
    for (const char character : kInfraredPrefix)
    {
        if (character == '\0')
        {
            break;
        }
        message[size++] = character;
    }
    size = appendUnsigned(message.data(), message.size(), size, sample.infrared);
    constexpr char kHeartRatePrefix[] = " HR=";
    for (const char character : kHeartRatePrefix)
    {
        if (character == '\0')
        {
            break;
        }
        message[size++] = character;
    }
    if (hasHeartRate)
    {
        size = appendUnsigned(message.data(), message.size(), size, heartRateBpm);
    }
    else
    {
        message[size++] = '-';
        message[size++] = '-';
    }
    message[size++] = '\r';
    message[size++] = '\n';
    return console.write(std::string_view(message.data(), size));
}

platform::Result<void> writeHeartRateError(platform::middleware::cli::IConsole& console,
                                           platform::ErrorCode error)
{
    std::array<char, 48U> message{};
    constexpr char kPrefix[] = "Heart-rate estimator error: ";
    std::size_t size{0U};
    for (const char character : kPrefix)
    {
        if (character == '\0')
        {
            break;
        }
        message[size++] = character;
    }
    size = appendUnsigned(message.data(), message.size(), size,
                          static_cast<std::uint32_t>(error));
    message[size++] = '\r';
    message[size++] = '\n';
    return console.write(std::string_view(message.data(), size));
}

void delayLoop(std::uint32_t iterations)
{
    for (volatile std::uint32_t i = 0U; i < iterations; ++i)
    {
        __asm volatile("nop");
    }
}

} // namespace

namespace app
{

Application::Application(platform::hal::IGpio& statusLed,
                         platform::middleware::cli::IConsole& console,
                         platform::hal::IFlashStorage& flash,
                         platform::drivers::max30102::Max30102* max30102)
    : statusLed_(statusLed)
    , console_(console)
    , flashCommands_(console, flash)
    , ledCommands_(console)
    , max30102_(max30102)
{
}

bool Application::initialize()
{
    platform::hal::GpioConfig config{};

    config.direction = platform::hal::GpioDirection::Output;

    config.pull = platform::hal::GpioPull::None;

    config.initialState = platform::hal::GpioState::High;

    config.interruptEdge = platform::hal::GpioInterruptEdge::None;

    const auto result = statusLed_.configure(config);

    if (!result)
    {
        return false;
    }

    if (max30102_ != nullptr)
    {
        const auto sensorResult = max30102_->begin();
        if (!sensorResult)
        {
            return false;
        }
        sensorOperational_ = true;
        sensorStreaming_ = true;
        heartRateEstimator_.reset();
        heartRateEstimatorOperational_ = true;
    }

    return true;
}

bool Application::blinkingEnabled() const noexcept
{
    return ledCommands_.blinkingEnabled();
}

int Application::run()
{
    if (!initialize())
    {
        while (true)
        {
        }
    }

    platform::middleware::cli::CommandRegistry commands;
    if (!ledCommands_.registerCommands(commands))
    {
        while (true)
        {
        }
    }
    if (!flashCommands_.registerCommands(commands))
    {
        while (true)
        {
        }
    }
    if (max30102_ != nullptr)
    {
        if (!commands.registerCommand("sensor on", &Application::enableSensorStreaming, this) ||
            !commands.registerCommand("sensor off", &Application::disableSensorStreaming, this))
        {
            while (true)
            {
            }
        }
    }

    platform::middleware::cli::ConsoleCli cli(console_, commands);
    if (!cli.start())
    {
        while (true)
        {
        }
    }

    bool ledOn = false;
    while (true)
    {
        for (std::uint32_t interval = 0U; interval < kCliPollsPerBlinkInterval; ++interval)
        {
            (void)cli.poll();
            pollSensor();
            delayLoop(kDelayIterationsPerCliPoll);
        }

        if (ledCommands_.blinkingEnabled())
        {
            ledOn = !ledOn;
            statusLed_.write(ledOn ? platform::hal::GpioState::Low
                                   : platform::hal::GpioState::High);
        }
        else if (ledOn)
        {
            ledOn = false;
            statusLed_.write(platform::hal::GpioState::High);
        }
    }

    return 0;
}

void Application::enableSensorStreaming(void* context)
{
    auto* application = static_cast<Application*>(context);
    if (!application->sensorOperational_ && application->max30102_ != nullptr)
    {
        const auto beginResult = application->max30102_->begin();
        if (!beginResult)
        {
            static_cast<void>(writeSensorError(application->console_, beginResult.error()));
            return;
        }
        application->sensorOperational_ = true;
    }

    if (application->sensorOperational_)
    {
        application->heartRateEstimator_.reset();
        application->heartRateEstimatorOperational_ = true;
        application->sensorStreaming_ = true;
        application->sensorErrorReported_ = false;
        application->sensorRetryPolls_ = 0U;
        static_cast<void>(application->console_.write("MAX30102 raw stream enabled\r\n"));
        return;
    }
    static_cast<void>(application->console_.write("MAX30102 is not available\r\n"));
}

void Application::disableSensorStreaming(void* context)
{
    auto* application = static_cast<Application*>(context);
    application->sensorStreaming_ = false;
    application->heartRateEstimator_.reset();
    static_cast<void>(application->console_.write("MAX30102 raw stream disabled\r\n"));
}

void Application::pollSensor()
{
    if (max30102_ == nullptr || !sensorOperational_)
    {
        return;
    }

    if (sensorRetryPolls_ > 0U)
    {
        --sensorRetryPolls_;
        return;
    }

    const auto result = max30102_->poll();
    if (!result)
    {
        if (!sensorErrorReported_)
        {
            static_cast<void>(writeSensorError(console_, result.error()));
            sensorErrorReported_ = true;
        }

        if (result.error() != platform::ErrorCode::Busy &&
            result.error() != platform::ErrorCode::Timeout &&
            result.error() != platform::ErrorCode::CommunicationError)
        {
            sensorOperational_ = false;
            sensorStreaming_ = false;
        }
        else
        {
            sensorRetryPolls_ = kCliPollsPerBlinkInterval;
        }
        return;
    }

    sensorErrorReported_ = false;
    sensorRetryPolls_ = 0U;
    while (max30102_->hasSample())
    {
        const auto sample = max30102_->readSample();
        if (!sample)
        {
            static_cast<void>(console_.write("MAX30102 sample read failed\r\n"));
            sensorOperational_ = false;
            sensorStreaming_ = false;
            return;
        }

        if (!sensorStreaming_)
        {
            continue;
        }

        if (heartRateEstimatorOperational_)
        {
            const auto heartRateResult = heartRateEstimator_.process(sample.value()->infrared);
            if (!heartRateResult)
            {
                static_cast<void>(writeHeartRateError(console_, heartRateResult.error()));
                heartRateEstimatorOperational_ = false;
            }
        }

        const auto writeResult =
            writeRawSample(console_, *sample.value(), heartRateEstimatorOperational_ &&
                                                          heartRateEstimator_.hasHeartRate(),
                           heartRateEstimator_.heartRateBpm());
        if (!writeResult)
        {
            sensorOperational_ = false;
            sensorStreaming_ = false;
            return;
        }
    }
}

} // namespace app