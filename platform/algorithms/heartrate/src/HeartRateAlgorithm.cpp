#include "platform/algorithms/heartrate/HeartRateAlgorithm.hpp"

#include <algorithm>

namespace platform::algorithms::heartrate
{
namespace
{

std::uint64_t integerSquareRoot(std::uint64_t value) noexcept
{
    std::uint64_t result{0U};
    std::uint64_t bit{1ULL << 62U};
    while (bit > value)
    {
        bit >>= 2U;
    }

    while (bit != 0U)
    {
        if (value >= result + bit)
        {
            value -= result + bit;
            result = (result >> 1U) + bit;
        }
        else
        {
            result >>= 1U;
        }
        bit >>= 2U;
    }
    return result;
}

std::size_t minimumInterval(const HeartRateEstimatorConfig& config) noexcept
{
    return (60U * config.sampleRateHz + config.maximumBpm - 1U) / config.maximumBpm;
}

std::size_t maximumInterval(const HeartRateEstimatorConfig& config) noexcept
{
    return (60U * config.sampleRateHz) / config.minimumBpm;
}

bool hasValidAnalysisRange(const HeartRateEstimatorConfig& config) noexcept
{
    if (config.sampleRateHz == 0U || config.minimumBpm == 0U ||
        config.maximumBpm == 0U || config.minimumBpm > config.maximumBpm)
    {
        return false;
    }

    const auto minimum = minimumInterval(config);
    const auto maximum = maximumInterval(config);
    return minimum >= 2U && minimum <= maximum &&
           maximum < (kHeartRateAnalysisWindowSamples / 2U);
}

} // namespace

std::size_t AutocorrelationHeartRateAlgorithm::estimateInterval(
    const std::array<std::int32_t, kHeartRateAnalysisWindowSamples>& samples,
    std::size_t nextSampleIndex, const HeartRateEstimatorConfig& config) const noexcept
{
    if (!hasValidAnalysisRange(config))
    {
        return 0U;
    }

    std::int64_t total{0};
    for (std::size_t index = 0U; index < samples.size(); ++index)
    {
        total += samples[(nextSampleIndex + index) % samples.size()];
    }

    const auto mean = total / static_cast<std::int64_t>(samples.size());
    std::uint64_t totalEnergy{0U};
    for (std::size_t index = 0U; index < samples.size(); ++index)
    {
        const auto value =
            static_cast<std::int64_t>(samples[(nextSampleIndex + index) % samples.size()]) - mean;
        totalEnergy += static_cast<std::uint64_t>(value * value);
    }
    const auto rms = integerSquareRoot(totalEnergy / samples.size());
    if (rms < config.minimumSignalAmplitude)
    {
        return 0U;
    }

    auto bestCorrelationPermille = std::int64_t{0};
    auto bestInterval = std::size_t{0U};
    for (std::size_t lag = minimumInterval(config); lag <= maximumInterval(config); ++lag)
    {
        const auto pairCount = samples.size() - lag;
        std::int64_t leftSum{0};
        std::int64_t rightSum{0};
        std::int64_t leftEnergy{0};
        std::int64_t rightEnergy{0};
        std::int64_t crossEnergy{0};

        for (std::size_t index = 0U; index < pairCount; ++index)
        {
            const auto left =
                static_cast<std::int64_t>(samples[(nextSampleIndex + index) % samples.size()]) -
                mean;
            const auto right = static_cast<std::int64_t>(
                                   samples[(nextSampleIndex + index + lag) % samples.size()]) -
                               mean;
            leftSum += left;
            rightSum += right;
            leftEnergy += left * left;
            rightEnergy += right * right;
            crossEnergy += left * right;
        }

        const auto count = static_cast<std::int64_t>(pairCount);
        leftEnergy -= (leftSum * leftSum) / count;
        rightEnergy -= (rightSum * rightSum) / count;
        crossEnergy -= (leftSum * rightSum) / count;
        if (leftEnergy <= 0 || rightEnergy <= 0 || crossEnergy <= 0)
        {
            continue;
        }

        const auto denominator =
            integerSquareRoot(static_cast<std::uint64_t>(leftEnergy)) *
            integerSquareRoot(static_cast<std::uint64_t>(rightEnergy));
        const auto correlationPermille = (crossEnergy * 1000) /
                                         static_cast<std::int64_t>(denominator);
        if (correlationPermille > bestCorrelationPermille)
        {
            bestCorrelationPermille = correlationPermille;
            bestInterval = lag;
        }
    }

    return bestCorrelationPermille >= config.minimumCorrelationPermille ? bestInterval : 0U;
}

std::size_t PeakDetectionHeartRateAlgorithm::estimateInterval(
    const std::array<std::int32_t, kHeartRateAnalysisWindowSamples>& samples,
    std::size_t nextSampleIndex, const HeartRateEstimatorConfig& config) const noexcept
{
    if (!hasValidAnalysisRange(config))
    {
        return 0U;
    }

    std::int64_t total{0};
    auto maximumPeak = std::int64_t{0};
    for (std::size_t index = 0U; index < samples.size(); ++index)
    {
        total += samples[(nextSampleIndex + index) % samples.size()];
    }
    const auto mean = total / static_cast<std::int64_t>(samples.size());
    for (std::size_t index = 0U; index < samples.size(); ++index)
    {
        const auto value =
            static_cast<std::int64_t>(samples[(nextSampleIndex + index) % samples.size()]) - mean;
        maximumPeak = std::max(maximumPeak, value);
    }

    const auto threshold =
        std::max<std::int64_t>(config.minimumSignalAmplitude, maximumPeak / 2);
    if (maximumPeak < threshold)
    {
        return 0U;
    }

    std::array<std::uint16_t, kHeartRateAnalysisWindowSamples / 2U> intervalCounts{};
    std::size_t intervalCount{0U};
    auto hasPreviousPeak = false;
    auto previousPeakIndex = std::size_t{0U};
    auto previousPeakValue = std::int64_t{0};
    const auto minimumPeakSpacing = minimumInterval(config);
    for (std::size_t index = 1U; index + 1U < samples.size(); ++index)
    {
        const auto previous =
            static_cast<std::int64_t>(samples[(nextSampleIndex + index - 1U) % samples.size()]) -
            mean;
        const auto current =
            static_cast<std::int64_t>(samples[(nextSampleIndex + index) % samples.size()]) - mean;
        const auto next =
            static_cast<std::int64_t>(samples[(nextSampleIndex + index + 1U) % samples.size()]) -
            mean;
        if (current < threshold || current < previous || current <= next)
        {
            continue;
        }

        if (hasPreviousPeak && index - previousPeakIndex < minimumPeakSpacing)
        {
            if (current > previousPeakValue)
            {
                previousPeakIndex = index;
                previousPeakValue = current;
            }
            continue;
        }
        if (hasPreviousPeak)
        {
            const auto interval = index - previousPeakIndex;
            if (interval >= minimumPeakSpacing && interval <= maximumInterval(config))
            {
                ++intervalCounts[interval];
                ++intervalCount;
            }
        }
        previousPeakIndex = index;
        previousPeakValue = current;
        hasPreviousPeak = true;
    }

    if (intervalCount == 0U)
    {
        return 0U;
    }

    auto intervalsBelowMedian = std::size_t{0U};
    for (std::size_t interval = minimumPeakSpacing; interval <= maximumInterval(config); ++interval)
    {
        intervalsBelowMedian += intervalCounts[interval];
        if (intervalsBelowMedian > intervalCount / 2U)
        {
            return interval;
        }
    }
    return 0U;
}

} // namespace platform::algorithms::heartrate
