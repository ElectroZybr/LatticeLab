#pragma once

#include <algorithm>
#include <cstdio>
#include <format>
#include <string>

#include <Lattice/Tools/BmRunner/BenchTypes.hpp>
#include "Lattice/Tools/BmRunner/Analysis.hpp"

namespace Lattice::Benchmarks::Output {

/**
 @file Output.hpp
 @brief Стандартный консольный вывод результатов бенчмарков.

 Форматирует промежуточные и итоговые результаты измерений
 для вывода в стандартный поток.
*/

inline std::string formatValue(double value, Unit unit) {
    switch (unit) {
        case Unit::Nanoseconds:
            if (value >= 1'000'000'000.0) return std::format("{:.2f} s", value / 1'000'000'000.0);
            if (value >= 1'000'000.0) return std::format("{:.2f} ms", value / 1'000'000.0);
            if (value >= 1'000.0) return std::format("{:.2f} us", value / 1'000.0);
            return std::format("{:.2f} ns", value);

        case Unit::Bytes:
            if (value >= 1024.0 * 1024.0 * 1024.0) return std::format("{:.2f} GiB", value / (1024.0 * 1024.0 * 1024.0));
            if (value >= 1024.0 * 1024.0) return std::format("{:.2f} MiB", value / (1024.0 * 1024.0));
            if (value >= 1024.0) return std::format("{:.2f} KiB", value / 1024.0);
            return std::format("{:.0f} B", value);

        case Unit::Percent: return std::format("{:.3f}%", value);
        case Unit::Ratio: return std::format("{:.3f}", value);
        case Unit::Count:
        case Unit::Cycles: return std::format("{:.0f}", value);
        case Unit::None: return std::format("{}", value);
    }

    return std::format("{}", value);
}

inline std::string fullName(std::string_view group, std::string_view name) {
    return group.empty()
        ? std::string(name)
        : std::format("{}/{}", group, name);
}

inline void printMetrics(
    FILE* out,
    const Metrics& metrics,
    bool liveOnly = false
) {
    const size_t count =
        std::min(metrics.schema.size(), metrics.values.size());

    for (size_t i = 0; i < count; ++i) {
        const MetricDesc& desc = metrics.schema[i];

        if (liveOnly && !hasFlag(desc.flags, MetricFlags::Live))
            continue;

        const std::string value =
            formatValue(metrics.values[i], desc.unit);

        std::fprintf(
            out,
            "  %.*s=%s",
            static_cast<int>(desc.name.size()),
            desc.name.data(),
            value.c_str()
        );
    }
}

inline void printMetrics(
    FILE* out,
    const std::vector<CapabilityMetrics>& capabilities,
    bool liveOnly = false
) {
    for (const CapabilityMetrics& capability : capabilities)
        printMetrics(out, capability.metrics, liveOnly);
}

struct LiveState {
    bool active = false;
};

inline LiveState& liveState() {
    static LiveState state;
    return state;
}

inline bool hasLiveMetrics(const std::vector<CapabilityMetrics>& capabilities) {
    for (const CapabilityMetrics& capability : capabilities) {
        const Metrics& metrics = capability.metrics;
        const size_t count = std::min(metrics.schema.size(), metrics.values.size());

        for (size_t i = 0; i < count; ++i)
            if (hasFlag(metrics.schema[i].flags, MetricFlags::Live))
                return true;
    }

    return false;
}

inline void sample(const SampleResult& sample) {
    if (!hasLiveMetrics(sample.capabilities))
        return;

    LiveState& state = liveState();

    if (sample.sample == 1) {
        if (state.active)
            std::fprintf(stdout, "\n");

        state.active = true;
    }

    const std::string name =
        fullName(sample.group, sample.name);

    std::fprintf(
        stdout,
        "\r\033[2K%-16s N=%-7zu %-12.*s",
        name.c_str(),
        sample.n,
        static_cast<int>(sample.stage.size()),
        sample.stage.data()
    );

    const std::string overhead = formatValue(sample.overhead, Unit::Nanoseconds);

    std::fprintf(
        stdout,
        " ovhd=%-10s",
        overhead.c_str()
    );
    
    std::fprintf(
        stdout,
        " stage=%2zu",
        sample.sample
    );

    if (sample.samples)
        std::fprintf(
            stdout,
            "/%-3zu",
            sample.samples
        );

    printMetrics(
        stdout,
        sample.capabilities,
        true
    );

    std::fflush(stdout);
}

inline void result(const PointResult&) {
    LiveState& state = liveState();

    if (!state.active)
        return;

    std::fprintf(stdout, "\n");
    std::fflush(stdout);

    state.active = false;
}

inline void complete(const BenchResult& result) {
    if (result.analysis.empty())
        return;

    for (const AnalysisResult& analysis : result.analysis) {
        switch (analysis.type) {
            case AnalysisType::Growth:
                std::fprintf(
                    stdout,
                    "Analysis  growth=%s  coefficient=%.6g  error=%.3f%%\n",
                    complexityName(analysis.bigO.complexity).data(),
                    analysis.bigO.coefficient,
                    analysis.bigO.error * 100.0
                );
                break;
            case AnalysisType::Correlation:
                break;
            }
    }

    std::fprintf(stdout, "\n");
    std::fflush(stdout);
}

}