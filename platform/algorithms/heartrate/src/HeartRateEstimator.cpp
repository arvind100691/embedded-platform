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
const AutocorrelationHeartRateAlgorithm kDefaultHeartRateAlgorithm{};

} // namespace

HeartRateEstimator::HeartRateEstimator(HeartRateEstimatorConfig config)
    : config_(config), algorithm_(&kDefaultHeartRateAlgorithm)
{
}

HeartRateEstimator::HeartRateEstimator(const IHeartRateAlgorithm& algorithm,
                                       HeartRateEstimatorConfig config)
    : config_(config), algorithm_(&algorithm)
{
}

void HeartRateEstimator::reset() noexcept
{
    hasHeartRate_ = false;
    baselineInitialized_ = false;
    baseline_ = 0;
    sampleWindow_.fill(0);
    nextSampleIndex_ = 0U;
    samplesInWindow_ = 0U;
    samplesSinceAnalysis_ = 0U;
    intervalHistory_.fill(0U);
    intervalHistoryCount_ = 0U;
    intervalHistoryNext_ = 0U;
    consecutiveGoodWindows_ = 0U;
    consecutiveWeakWindows_ = 0U;
    heartRateBpm_ = 0U;
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
    const auto minimumInterval =
        (60U * config_.sampleRateHz + config_.maximumBpm - 1U) / config_.maximumBpm;
    const auto maximumInterval = (60U * config_.sampleRateHz) / config_.minimumBpm;
    const auto bestInterval =
        algorithm_->estimateInterval(sampleWindow_, nextSampleIndex_, config_);

    if (bestInterval < minimumInterval || bestInterval > maximumInterval)
    {
        if (consecutiveWeakWindows_ < kWeakWindowsBeforeReset)
        {
            ++consecutiveWeakWindows_;
        }
        if (consecutiveWeakWindows_ >= kWeakWindowsBeforeReset)
        {
            intervalHistoryCount_ = 0U;
            intervalHistoryNext_ = 0U;
            consecutiveGoodWindows_ = 0U;
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
