#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "platform/algorithms/heartrate/HeartRateEstimator.hpp"

namespace
{

using platform::algorithms::heartrate::HeartRateEstimator;
using platform::algorithms::heartrate::HeartRateEstimatorConfig;

void processSamples(HeartRateEstimator& estimator, const std::vector<std::uint32_t>& samples)
{
    for (const auto sample : samples)
    {
        ASSERT_TRUE(estimator.process(sample));
    }
}

} // namespace

TEST(HeartRateEstimatorTest, EstimatesRateFromPeriodicIrPeaks)
{
    HeartRateEstimatorConfig config{};
    config.maximumBpm = 200U;
    HeartRateEstimator estimator(config);
    std::vector<std::uint32_t> samples;
    for (std::size_t index = 0U; index < 1400U; ++index)
    {
        const auto phase = index % 50U;
        samples.push_back(100000U + (phase == 10U ? 20000U : 0U));
    }

    processSamples(estimator, samples);

    ASSERT_TRUE(estimator.hasHeartRate());
    EXPECT_EQ(estimator.heartRateBpm(), 120U);
}

TEST(HeartRateEstimatorTest, DetectsLowerAmplitudeIrPeaks)
{
    HeartRateEstimatorConfig config{};
    config.maximumBpm = 200U;
    config.minimumSignalAmplitude = 50U;
    HeartRateEstimator estimator(config);
    std::vector<std::uint32_t> samples;
    for (std::size_t index = 0U; index < 1400U; ++index)
    {
        const auto phase = index % 50U;
        samples.push_back(206000U + (phase == 10U ? 600U : 0U));
    }

    processSamples(estimator, samples);

    ASSERT_TRUE(estimator.hasHeartRate());
    EXPECT_EQ(estimator.heartRateBpm(), 120U);
}

TEST(HeartRateEstimatorTest, RejectsSecondaryPeaksWithinOnePulse)
{
    HeartRateEstimator estimator;
    std::vector<std::uint32_t> samples;
    for (std::size_t index = 0U; index < 1700U; ++index)
    {
        const auto phase = index % 92U;
        const auto pulse = phase == 10U ? 12000U : phase == 45U ? 5000U : 0U;
        samples.push_back(100000U + pulse);
    }

    processSamples(estimator, samples);

    ASSERT_TRUE(estimator.hasHeartRate());
    EXPECT_NEAR(estimator.heartRateBpm(), 65U, 1U);
}

TEST(HeartRateEstimatorTest, SmoothsBeatToBeatIntervalJitter)
{
    HeartRateEstimator estimator;
    constexpr std::array<std::size_t, 6U> kIntervals{90U, 110U, 100U, 95U, 105U, 100U};
    std::vector<std::uint32_t> samples(1800U, 100000U);
    std::size_t peakIndex{10U};
    std::size_t intervalIndex{0U};
    while (peakIndex < samples.size())
    {
        samples[peakIndex] += 12000U;
        peakIndex += kIntervals[intervalIndex % kIntervals.size()];
        ++intervalIndex;
    }

    processSamples(estimator, samples);

    ASSERT_TRUE(estimator.hasHeartRate());
    EXPECT_GE(estimator.heartRateBpm(), 57U);
    EXPECT_LE(estimator.heartRateBpm(), 63U);
}

TEST(HeartRateEstimatorTest, MaximumRateCanBeRaisedForFasterSignals)
{
    HeartRateEstimatorConfig config{};
    config.maximumBpm = 200U;
    HeartRateEstimator estimator(config);
    std::vector<std::uint32_t> samples;
    for (std::size_t index = 0U; index < 1400U; ++index)
    {
        samples.push_back(100000U + ((index % 50U) == 10U ? 20000U : 0U));
    }

    processSamples(estimator, samples);

    ASSERT_TRUE(estimator.hasHeartRate());
    EXPECT_EQ(estimator.heartRateBpm(), 120U);
}

TEST(HeartRateEstimatorTest, DoesNotReportNoiseAsAHeartRate)
{
    HeartRateEstimator estimator;
    std::vector<std::uint32_t> samples;
    for (std::size_t index = 0U; index < 1400U; ++index)
    {
        const auto noise = static_cast<std::uint32_t>((index * 37U + index * index * 13U) % 181U);
        samples.push_back(100000U + noise);
    }

    processSamples(estimator, samples);

    EXPECT_FALSE(estimator.hasHeartRate());
}

TEST(HeartRateEstimatorTest, RejectsInvalidConfiguration)
{
    HeartRateEstimatorConfig config{};
    config.sampleRateHz = 0U;
    HeartRateEstimator estimator(config);

    const auto result = estimator.process(100000U);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::InvalidArgument);
}

TEST(HeartRateEstimatorTest, RejectsZeroMaximumRate)
{
    HeartRateEstimatorConfig config{};
    config.maximumBpm = 0U;
    HeartRateEstimator estimator(config);

    const auto result = estimator.process(100000U);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), platform::ErrorCode::InvalidArgument);
}

TEST(HeartRateEstimatorTest, ResetClearsPreviousRate)
{
    HeartRateEstimator estimator;
    std::vector<std::uint32_t> samples;
    for (std::size_t index = 0U; index < 1200U; ++index)
    {
        samples.push_back(100000U + ((index % 100U) == 10U ? 20000U : 0U));
    }
    processSamples(estimator, samples);
    ASSERT_TRUE(estimator.hasHeartRate());

    estimator.reset();

    EXPECT_FALSE(estimator.hasHeartRate());
}
