#include "platform/algorithms/heartrate/HeartRateEstimator.hpp"

#include <algorithm>
#include <limits>

#include "platform/common/Result.hpp"

namespace platform::algorithms::heartrate
{
namespace
{

constexpr std::size_t kBaselineDivisor{64U};
constexpr std::size_t kSamplesBetweenAnalyses{100U};
constexpr std::uint16_t kStableWindowsRequired{3U};
constexpr std::uint16_t kWeakWindowsBeforeReset{3U};
constexpr std::uint32_t kIntervalChangePercent{10U};

std::uint64_t integerSquareRoot(std::uint64_t value)
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

} // namespace

HeartRateEstimator::HeartRateEstimator(HeartRateEstimatorConfig config)
    : config_(config)
{
}

void HeartRateEstimator::reset() noexcept
{
    *this = HeartRateEstimator(config_);
}

platform::Result<void> HeartRateEstimator::process(std::uint32_t infraredSample)
{
    if (config_.sampleRateHz == 0U || config_.minimumBpm == 0U ||
        config_.maximumBpm == 0U ||
        config_.minimumBpm > config_.maximumBpm ||
        config_.minimumCorrelationPermille > 1000U)
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    const auto minimumInterval =
        (60U * config_.sampleRateHz + config_.maximumBpm - 1U) / config_.maximumBpm;
    const auto maximumInterval = (60U * config_.sampleRateHz) / config_.minimumBpm;
    if (minimumInterval < 2U || maximumInterval >= (kAnalysisWindowSamples / 2U))
    {
        return platform::Result<void>::failure(platform::ErrorCode::InvalidArgument);
    }

    const auto sample = static_cast<std::int64_t>(infraredSample);
    if (!baselineInitialized_)
    {
        baseline_ = sample;
        baselineInitialized_ = true;
    }
    baseline_ += (sample - baseline_) / static_cast<std::int64_t>(kBaselineDivisor);
    const auto filteredValue = static_cast<std::int32_t>(sample - baseline_);

    sampleWindow_[nextSampleIndex_] = filteredValue;
    nextSampleIndex_ = (nextSampleIndex_ + 1U) % sampleWindow_.size();
    if (samplesInWindow_ < sampleWindow_.size())
    {
        ++samplesInWindow_;
    }

    if (samplesInWindow_ == sampleWindow_.size())
    {
        ++samplesSinceAnalysis_;
        if (samplesSinceAnalysis_ >= kSamplesBetweenAnalyses)
        {
            samplesSinceAnalysis_ = 0U;
            analyzeWindow();
        }
    }
    return platform::Result<void>::success();
}

void HeartRateEstimator::analyzeWindow()
{
    std::int64_t total{0};
    for (std::size_t index = 0U; index < sampleWindow_.size(); ++index)
    {
        const auto ringIndex = (nextSampleIndex_ + index) % sampleWindow_.size();
        total += sampleWindow_[ringIndex];
    }

    const auto mean = total / static_cast<std::int64_t>(sampleWindow_.size());
    std::uint64_t totalEnergy{0U};
    for (std::size_t index = 0U; index < sampleWindow_.size(); ++index)
    {
        const auto ringIndex = (nextSampleIndex_ + index) % sampleWindow_.size();
        const auto value = static_cast<std::int64_t>(sampleWindow_[ringIndex]) - mean;
        totalEnergy += static_cast<std::uint64_t>(value * value);
    }
    const auto rms = integerSquareRoot(totalEnergy / sampleWindow_.size());

    const auto minimumInterval =
        (60U * config_.sampleRateHz + config_.maximumBpm - 1U) / config_.maximumBpm;
    const auto maximumInterval = (60U * config_.sampleRateHz) / config_.minimumBpm;
    std::int64_t bestCorrelationPermille{0};
    std::size_t bestInterval{0U};

    if (rms >= config_.minimumSignalAmplitude)
    {
        for (std::size_t lag = minimumInterval; lag <= maximumInterval; ++lag)
        {
            const auto pairCount = sampleWindow_.size() - lag;
            std::int64_t leftSum{0};
            std::int64_t rightSum{0};
            std::int64_t leftEnergy{0};
            std::int64_t rightEnergy{0};
            std::int64_t crossEnergy{0};

            for (std::size_t index = 0U; index < pairCount; ++index)
            {
                const auto leftIndex = (nextSampleIndex_ + index) % sampleWindow_.size();
                const auto rightIndex =
                    (nextSampleIndex_ + index + lag) % sampleWindow_.size();
                const auto left =
                    static_cast<std::int64_t>(sampleWindow_[leftIndex]) - mean;
                const auto right =
                    static_cast<std::int64_t>(sampleWindow_[rightIndex]) - mean;
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
    }

    if (bestInterval == 0U ||
        bestCorrelationPermille < config_.minimumCorrelationPermille)
    {
        if (consecutiveWeakWindows_ < kWeakWindowsBeforeReset)
        {
            ++consecutiveWeakWindows_;
        }
        consecutiveGoodWindows_ = 0U;
        if (consecutiveWeakWindows_ >= kWeakWindowsBeforeReset)
        {
            intervalHistoryCount_ = 0U;
            intervalHistoryNext_ = 0U;
            hasHeartRate_ = false;
        }
        return;
    }

    consecutiveWeakWindows_ = 0U;
    if (intervalHistoryCount_ > 0U)
    {
        auto sortedIntervals = intervalHistory_;
        std::sort(sortedIntervals.begin(),
                  sortedIntervals.begin() +
                      static_cast<std::ptrdiff_t>(intervalHistoryCount_));
        const auto medianInterval = sortedIntervals[intervalHistoryCount_ / 2U];
        const auto difference = bestInterval > medianInterval
                                    ? bestInterval - medianInterval
                                    : medianInterval - bestInterval;
        if (difference * 100U > medianInterval * kIntervalChangePercent)
        {
            intervalHistoryCount_ = 0U;
            intervalHistoryNext_ = 0U;
            consecutiveGoodWindows_ = 0U;
        }
    }

    intervalHistory_[intervalHistoryNext_] = static_cast<std::uint32_t>(bestInterval);
    intervalHistoryNext_ = (intervalHistoryNext_ + 1U) % intervalHistory_.size();
    if (intervalHistoryCount_ < intervalHistory_.size())
    {
        ++intervalHistoryCount_;
    }
    if (consecutiveGoodWindows_ < kStableWindowsRequired)
    {
        ++consecutiveGoodWindows_;
    }
    if (consecutiveGoodWindows_ < kStableWindowsRequired)
    {
        return;
    }

    auto sortedIntervals = intervalHistory_;
    std::sort(sortedIntervals.begin(),
              sortedIntervals.begin() +
                  static_cast<std::ptrdiff_t>(intervalHistoryCount_));
    const auto medianInterval = sortedIntervals[intervalHistoryCount_ / 2U];
    const auto bpm = (60U * config_.sampleRateHz + medianInterval / 2U) / medianInterval;
    heartRateBpm_ = static_cast<std::uint16_t>(
        std::min<std::uint32_t>(bpm, std::numeric_limits<std::uint16_t>::max()));
    hasHeartRate_ = true;
}

bool HeartRateEstimator::hasHeartRate() const noexcept
{
    return hasHeartRate_;
}

std::uint16_t HeartRateEstimator::heartRateBpm() const noexcept
{
    return heartRateBpm_;
}

} // namespace platform::algorithms::heartrate
