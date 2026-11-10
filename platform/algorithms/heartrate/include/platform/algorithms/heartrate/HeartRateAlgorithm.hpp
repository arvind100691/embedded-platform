#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace platform::algorithms::heartrate
{

constexpr std::size_t kHeartRateAnalysisWindowSamples{768U};

struct HeartRateEstimatorConfig
{
    std::uint16_t sampleRateHz{100U};
    std::uint16_t minimumBpm{40U};
    std::uint16_t maximumBpm{180U};
    std::uint32_t minimumSignalAmplitude{100U};
    std::uint16_t minimumCorrelationPermille{350U};
};

class IHeartRateAlgorithm
{
public:
    virtual ~IHeartRateAlgorithm() = default;

    [[nodiscard]] virtual std::size_t estimateInterval(
        const std::array<std::int32_t, kHeartRateAnalysisWindowSamples>& samples,
        std::size_t nextSampleIndex, const HeartRateEstimatorConfig& config) const noexcept = 0;
};

class AutocorrelationHeartRateAlgorithm final : public IHeartRateAlgorithm
{
public:
    [[nodiscard]] std::size_t estimateInterval(
        const std::array<std::int32_t, kHeartRateAnalysisWindowSamples>& samples,
        std::size_t nextSampleIndex, const HeartRateEstimatorConfig& config) const noexcept override;
};

class PeakDetectionHeartRateAlgorithm final : public IHeartRateAlgorithm
{
public:
    [[nodiscard]] std::size_t estimateInterval(
        const std::array<std::int32_t, kHeartRateAnalysisWindowSamples>& samples,
        std::size_t nextSampleIndex, const HeartRateEstimatorConfig& config) const noexcept override;
};

} // namespace platform::algorithms::heartrate
