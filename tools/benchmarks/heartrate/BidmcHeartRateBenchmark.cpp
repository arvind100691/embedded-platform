#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "platform/algorithms/heartrate/HeartRateAlgorithm.hpp"
#include "platform/algorithms/heartrate/HeartRateEstimator.hpp"

namespace
{

using platform::algorithms::heartrate::HeartRateEstimator;
using platform::algorithms::heartrate::HeartRateEstimatorConfig;
using platform::algorithms::heartrate::PeakDetectionHeartRateAlgorithm;

constexpr std::size_t kRecordingCount{53U};
constexpr std::size_t kStrategyCount{2U};
constexpr std::size_t kReferenceCount{2U};
constexpr std::size_t kSampleRateHz{125U};
constexpr double kSignalScale{1'000'000.0};
constexpr double kSignalOffset{2.0};
constexpr double kTimeToleranceSeconds{0.000001};

struct NumericReference
{
    double timeSeconds;
    double pulseRateBpm;
    double ecgHeartRateBpm;
};

struct Metrics
{
    std::uint64_t referenceCount{0U};
    std::uint64_t estimateCount{0U};
    std::uint64_t withinFiveBpmCount{0U};
    double absoluteErrorSum{0.0};
    double signedErrorSum{0.0};

    void add(double estimate, double reference)
    {
        const auto error = estimate - reference;
        ++estimateCount;
        absoluteErrorSum += std::abs(error);
        signedErrorSum += error;
        if (std::abs(error) <= 5.0)
        {
            ++withinFiveBpmCount;
        }
    }

    void print(const char* record, const char* strategy, const char* referenceName) const
    {
        const auto coverage = referenceCount == 0U
                                  ? 0.0
                                  : 100.0 * static_cast<double>(estimateCount) /
                                        static_cast<double>(referenceCount);
        const auto meanAbsoluteError =
            estimateCount == 0U ? std::numeric_limits<double>::quiet_NaN()
                                : absoluteErrorSum / static_cast<double>(estimateCount);
        const auto bias = estimateCount == 0U
                              ? std::numeric_limits<double>::quiet_NaN()
                              : signedErrorSum / static_cast<double>(estimateCount);
        const auto withinFivePercent =
            estimateCount == 0U
                ? 0.0
                : 100.0 * static_cast<double>(withinFiveBpmCount) /
                      static_cast<double>(estimateCount);
        std::cout << record << ',' << strategy << ',' << referenceName << ','
                  << referenceCount << ',' << estimateCount << ',' << std::fixed
                  << std::setprecision(1) << coverage << ',' << meanAbsoluteError << ','
                  << bias << ',' << withinFivePercent << '\n';
    }
};

std::vector<std::string> splitCsvLine(const std::string& line)
{
    std::vector<std::string> fields;
    std::istringstream input(line);
    std::string field;
    while (std::getline(input, field, ','))
    {
        const auto first = field.find_first_not_of(" \t\r\n");
        const auto last = field.find_last_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            fields.emplace_back();
            continue;
        }
        field = field.substr(first, last - first + 1U);
        fields.push_back(field);
    }
    return fields;
}

std::size_t findColumn(const std::vector<std::string>& header, const std::string& name)
{
    for (std::size_t index = 0U; index < header.size(); ++index)
    {
        if (header[index] == name)
        {
            return index;
        }
    }
    throw std::runtime_error("Missing CSV column: " + name);
}

double parseNumber(const std::string& value, const std::filesystem::path& path,
                   std::size_t lineNumber)
{
    std::size_t parsedLength{0U};
    try
    {
        const auto parsed = std::stod(value, &parsedLength);
        if (parsedLength != value.size() && value.substr(parsedLength).find_first_not_of("\r\t ") !=
                                                std::string::npos)
        {
            throw std::runtime_error("Invalid numeric field");
        }
        return parsed;
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("Invalid number in " + path.string() + ":" +
                                 std::to_string(lineNumber));
    }
}

std::vector<NumericReference> loadNumericReferences(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("Cannot open " + path.string());
    }

    std::string line;
    if (!std::getline(input, line))
    {
        throw std::runtime_error("Empty CSV file: " + path.string());
    }
    const auto header = splitCsvLine(line);
    const auto timeColumn = findColumn(header, "Time [s]");
    const auto pulseColumn = findColumn(header, "PULSE");
    const auto heartRateColumn = findColumn(header, "HR");
    std::vector<NumericReference> references;
    std::size_t lineNumber{1U};
    while (std::getline(input, line))
    {
        ++lineNumber;
        if (line.empty())
        {
            continue;
        }
        const auto fields = splitCsvLine(line);
        if (fields.size() <= std::max({timeColumn, pulseColumn, heartRateColumn}))
        {
            throw std::runtime_error("Missing numeric field in " + path.string() + ":" +
                                     std::to_string(lineNumber));
        }
        references.push_back({parseNumber(fields[timeColumn], path, lineNumber),
                              parseNumber(fields[pulseColumn], path, lineNumber),
                              parseNumber(fields[heartRateColumn], path, lineNumber)});
    }
    return references;
}

void processRecording(const std::filesystem::path& directory, std::size_t recordingIndex,
                      std::array<std::array<Metrics, kReferenceCount>, kStrategyCount>& totals)
{
    std::ostringstream recordName;
    recordName << "bidmc_" << std::setw(2) << std::setfill('0') << recordingIndex;
    const auto signalPath = directory / (recordName.str() + "_Signals.csv");
    const auto numericPath = directory / (recordName.str() + "_Numerics.csv");
    if (!std::filesystem::exists(signalPath) && !std::filesystem::exists(numericPath))
    {
        return;
    }
    if (!std::filesystem::exists(signalPath) || !std::filesystem::exists(numericPath))
    {
        throw std::runtime_error("Both Signals.csv and Numerics.csv are required for " +
                                 recordName.str());
    }

    const auto references = loadNumericReferences(numericPath);
    std::array<std::array<Metrics, kReferenceCount>, kStrategyCount> recordMetrics{};
    HeartRateEstimatorConfig config{};
    config.sampleRateHz = static_cast<std::uint16_t>(kSampleRateHz);
    HeartRateEstimator autocorrelationEstimator(config);
    PeakDetectionHeartRateAlgorithm peakDetectionAlgorithm;
    HeartRateEstimator peakDetectionEstimator(peakDetectionAlgorithm, config);
    std::array<HeartRateEstimator*, kStrategyCount> estimators{
        &autocorrelationEstimator, &peakDetectionEstimator};

    std::ifstream input(signalPath);
    if (!input)
    {
        throw std::runtime_error("Cannot open " + signalPath.string());
    }
    std::string line;
    if (!std::getline(input, line))
    {
        throw std::runtime_error("Empty CSV file: " + signalPath.string());
    }
    const auto header = splitCsvLine(line);
    const auto timeColumn = findColumn(header, "Time [s]");
    const auto plethColumn = findColumn(header, "PLETH");

    std::size_t referenceIndex{0U};
    std::size_t lineNumber{1U};
    while (std::getline(input, line))
    {
        ++lineNumber;
        if (line.empty())
        {
            continue;
        }
        const auto fields = splitCsvLine(line);
        if (fields.size() <= std::max(timeColumn, plethColumn))
        {
            throw std::runtime_error("Missing signal field in " + signalPath.string() + ":" +
                                     std::to_string(lineNumber));
        }
        const auto time = parseNumber(fields[timeColumn], signalPath, lineNumber);
        const auto pleth = parseNumber(fields[plethColumn], signalPath, lineNumber);
        if (!std::isfinite(time) || !std::isfinite(pleth))
        {
            continue;
        }
        const auto scaledSignal = (pleth + kSignalOffset) * kSignalScale;
        if (scaledSignal < 0.0 ||
            scaledSignal > static_cast<double>(std::numeric_limits<std::uint32_t>::max()))
        {
            throw std::runtime_error("PLETH value outside supported range in " +
                                     signalPath.string() + ":" + std::to_string(lineNumber));
        }
        const auto sample = static_cast<std::uint32_t>(std::llround(scaledSignal));
        for (auto* estimator : estimators)
        {
            const auto result = estimator->process(sample);
            if (!result)
            {
                throw std::runtime_error("Heart-rate estimator rejected benchmark configuration");
            }
        }

        while (referenceIndex < references.size() &&
               references[referenceIndex].timeSeconds <= time + kTimeToleranceSeconds)
        {
            const auto& reference = references[referenceIndex];
            const std::array<double, kReferenceCount> referenceRates{
                reference.pulseRateBpm, reference.ecgHeartRateBpm};
            for (std::size_t referenceKind = 0U; referenceKind < kReferenceCount;
                 ++referenceKind)
            {
                if (!std::isfinite(referenceRates[referenceKind]) ||
                    referenceRates[referenceKind] <= 0.0)
                {
                    continue;
                }
                for (std::size_t strategy = 0U; strategy < kStrategyCount; ++strategy)
                {
                    auto& metrics = recordMetrics[strategy][referenceKind];
                    ++metrics.referenceCount;
                    if (estimators[strategy]->hasHeartRate())
                    {
                        metrics.add(estimators[strategy]->heartRateBpm(),
                                    referenceRates[referenceKind]);
                    }
                }
            }
            ++referenceIndex;
        }
    }

    if (referenceIndex < references.size())
    {
        std::cerr << "Warning: " << references.size() - referenceIndex
                  << " reference timestamps were not covered by " << recordName.str() << '\n';
    }

    constexpr std::array<const char*, kStrategyCount> kStrategyNames{
        "autocorrelation", "peak-detection"};
    constexpr std::array<const char*, kReferenceCount> kReferenceNames{"PULSE", "ECG_HR"};
    std::cout << std::fixed << std::setprecision(1);
    for (std::size_t strategy = 0U; strategy < kStrategyCount; ++strategy)
    {
        for (std::size_t referenceKind = 0U; referenceKind < kReferenceCount; ++referenceKind)
        {
            recordMetrics[strategy][referenceKind].print(
                recordName.str().c_str(), kStrategyNames[strategy], kReferenceNames[referenceKind]);
            totals[strategy][referenceKind].referenceCount +=
                recordMetrics[strategy][referenceKind].referenceCount;
            totals[strategy][referenceKind].estimateCount +=
                recordMetrics[strategy][referenceKind].estimateCount;
            totals[strategy][referenceKind].withinFiveBpmCount +=
                recordMetrics[strategy][referenceKind].withinFiveBpmCount;
            totals[strategy][referenceKind].absoluteErrorSum +=
                recordMetrics[strategy][referenceKind].absoluteErrorSum;
            totals[strategy][referenceKind].signedErrorSum +=
                recordMetrics[strategy][referenceKind].signedErrorSum;
        }
    }
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: bidmc_heart_rate_benchmark <bidmc_csv_directory>\n";
        return 2;
    }

    try
    {
        const std::filesystem::path directory(argv[1]);
        std::array<std::array<Metrics, kReferenceCount>, kStrategyCount> totals{};
        std::size_t processedRecordings{0U};
        std::cout << "record,strategy,reference,reference_points,estimates,coverage_pct,"
                     "mae_bpm,bias_bpm,within_5bpm_pct\n";
        for (std::size_t recordingIndex = 1U; recordingIndex <= kRecordingCount;
             ++recordingIndex)
        {
            std::ostringstream signalName;
            signalName << "bidmc_" << std::setw(2) << std::setfill('0') << recordingIndex
                       << "_Signals.csv";
            if (!std::filesystem::exists(directory / signalName.str()))
            {
                continue;
            }
            processRecording(directory, recordingIndex, totals);
            ++processedRecordings;
        }
        if (processedRecordings == 0U)
        {
            throw std::runtime_error("No BIDMC *_Signals.csv files found in " +
                                     directory.string());
        }

        constexpr std::array<const char*, kStrategyCount> kStrategyNames{
            "autocorrelation", "peak-detection"};
        constexpr std::array<const char*, kReferenceCount> kReferenceNames{"PULSE", "ECG_HR"};
        for (std::size_t strategy = 0U; strategy < kStrategyCount; ++strategy)
        {
            for (std::size_t referenceKind = 0U; referenceKind < kReferenceCount;
                 ++referenceKind)
            {
                totals[strategy][referenceKind].print(
                    "ALL", kStrategyNames[strategy], kReferenceNames[referenceKind]);
            }
        }
        std::cerr << "Processed " << processedRecordings << " BIDMC recordings at "
                  << kSampleRateHz << " Hz.\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << "BIDMC benchmark failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
