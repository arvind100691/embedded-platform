#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "platform/algorithms/heartrate/HeartRateAlgorithm.hpp"
#include "platform/common/Result.hpp"

namespace platform::algorithms::heartrate
{

class HeartRateEstimator final
{
public:
    static constexpr std::size_t kAnalysisWindowSamples{kHeartRateAnalysisWindowSamples};
    static constexpr std::size_t kIntervalHistoryCapacity{5U};

    explicit HeartRateEstimator(HeartRateEstimatorConfig config = {});
    HeartRateEstimator(const IHeartRateAlgorithm& algorithm,
                       HeartRateEstimatorConfig config = {});

    void reset() noexcept;
    platform::Result<void> process(std::uint32_t infraredSample);

    [[nodiscard]] bool hasHeartRate() const noexcept;
    [[nodiscard]] std::uint16_t heartRateBpm() const noexcept;

private:
    void analyzeWindow();

    HeartRateEstimatorConfig config_;
    const IHeartRateAlgorithm* algorithm_;
    bool hasHeartRate_{false};
    bool baselineInitialized_{false};
    std::int64_t baseline_{0};
    std::array<std::int32_t, kAnalysisWindowSamples> sampleWindow_{};
    std::size_t nextSampleIndex_{0U};
    std::size_t samplesInWindow_{0U};
    std::size_t samplesSinceAnalysis_{0U};
    std::array<std::uint32_t, kIntervalHistoryCapacity> intervalHistory_{};
    std::size_t intervalHistoryCount_{0U};
    std::size_t intervalHistoryNext_{0U};
    std::size_t consecutiveGoodWindows_{0U};
    std::size_t consecutiveWeakWindows_{0U};
    std::uint16_t heartRateBpm_{0U};
};

} // namespace platform::algorithms::heartrate
