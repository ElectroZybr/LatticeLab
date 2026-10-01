#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include <string>
#include <vector>


namespace Lattice::Benchmarks {

enum class Unit : uint8_t {
    None,
    Count,
    Nanoseconds,
    Bytes,
    BytesPerSecond,
    Ratio,
    Percent,
    Cycles
};

enum class MetricFlags : uint8_t {
    None = 0,
    PerIteration = 1 << 0,
    Live = 1 << 1
};

constexpr MetricFlags operator|(MetricFlags a, MetricFlags b) {
    return static_cast<MetricFlags>(
        static_cast<uint8_t>(a) |
        static_cast<uint8_t>(b)
    );
}

constexpr bool hasFlag(MetricFlags value, MetricFlags flag) {
    return (
        static_cast<uint8_t>(value) &
        static_cast<uint8_t>(flag)
    ) != 0;
}

struct MetricDesc {
    std::string_view name;
    Unit unit = Unit::None;
    MetricFlags flags = MetricFlags::None;
};

struct Metrics {
    std::span<const MetricDesc> schema;
    std::vector<double> values;
};

struct CapabilityMetrics {
    std::string_view capability;
    Metrics metrics;
};

struct UnavailableCapability {
    std::string capability;
    std::string reason;
};

// Хранит результат прогона одного семпла
struct SampleResult {
    std::string_view name;
    std::string_view group;
    std::string_view stage;

    size_t n = 0;
    size_t sample = 0;
    size_t samples = 0;
    size_t iterations = 0;
    double overhead = 0;

    std::vector<CapabilityMetrics> capabilities;
};

struct StageResult {
    std::string name;
    size_t sample = 0;
    size_t samples = 0;
    double overhead = 0;
    std::vector<CapabilityMetrics> capabilities;
    std::vector<UnavailableCapability> unavailable;
};

// Хранит результат прогона одного поинта по N
struct PointResult {
    std::string_view name;
    std::string_view group;

    size_t n = 0;
    std::vector<StageResult> stages;
};

// результат полностью завершенного бенчмарка
struct BenchResult;

using SampleCallback = std::function<void(const SampleResult&)>;
using ResultCallback = std::function<void(const PointResult&)>;
using CompleteCallback = std::function<void(const BenchResult&)>;

}
