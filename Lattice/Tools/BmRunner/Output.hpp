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

        case Unit::BytesPerSecond:
            if (value >= 1024.0 * 1024.0 * 1024.0) return std::format("{:.2f} GiB/s", value / (1024.0 * 1024.0 * 1024.0));
            if (value >= 1024.0 * 1024.0) return std::format("{:.2f} MiB/s", value / (1024.0 * 1024.0));
            if (value >= 1024.0) return std::format("{:.2f} KiB/s", value / 1024.0);
            return std::format("{:.0f} B/s", value);

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

struct AnalysisValueInfo {
    std::string name;
    Unit unit = Unit::None;
};

inline AnalysisValueInfo analysisValueInfo(
    const BenchResult& result,
    ValueRef ref
) {
    if (ref.source == ValueSource::Parameter) {
        switch (ref.index) {
            case Metric::N:
                return {"N", Unit::None};
            default:
                return {std::format("parameter[{}]", ref.index), Unit::None};
        }
    }

    for (const PointResult& point : result.points) {
        for (const StageResult& stage : point.stages) {
            for (const CapabilityMetrics& capability : stage.capabilities) {
                if (
                    capability.capability == ref.capability &&
                    ref.index < capability.metrics.schema.size()
                ) {
                    const MetricDesc& metric = capability.metrics.schema[ref.index];

                    return {
                        std::format("{}.{}", ref.capability, metric.name),
                        metric.unit
                    };
                }
            }
        }
    }

    return {
        std::format("{}[{}]", ref.capability, ref.index),
        Unit::None
    };
}

inline std::string formatCoefficient(double value, Unit unit) {
    std::string_view suffix;

    switch (unit) {
        case Unit::Nanoseconds:    suffix = "ns"; break;
        case Unit::Bytes:          suffix = "B"; break;
        case Unit::BytesPerSecond: suffix = "B/s"; break;
        case Unit::Percent:        suffix = "%"; break;
        case Unit::Cycles:         suffix = "cycles"; break;
        case Unit::None:
        case Unit::Count:
        case Unit::Ratio:          break;
    }

    return suffix.empty()
        ? std::format("{:.3f}", value)
        : std::format("{:.3f} {}", value, suffix);
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
        " sample=%2zu",
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

inline void result(const PointResult& point) {
    LiveState& state = liveState();

    if (state.active)
        std::fprintf(stdout, "\r\033[2K");

    state.active = false;

    const std::string name =
        fullName(point.group, point.name);

    for (const StageResult& stage : point.stages) {
        if (hasLiveMetrics(stage.capabilities)) {
            const std::string overhead =
                formatValue(stage.overhead, Unit::Nanoseconds);

            std::fprintf(
                stdout,
                "%-16s N=%-7zu %-12s ovhd=%-10s sample=%2zu",
                name.c_str(),
                point.n,
                stage.name.c_str(),
                overhead.c_str(),
                stage.sample
            );

            if (stage.samples) {
                std::fprintf(
                    stdout,
                    "/%-3zu",
                    stage.samples
                );
            }

            printMetrics(
                stdout,
                stage.capabilities,
                true
            );

            std::fprintf(stdout, "\n");
        }

        for (const UnavailableCapability& unavailable : stage.unavailable) {
            std::fprintf(
                stdout,
                "%-16s N=%-7zu %-12s unavailable: %s\n",
                name.c_str(),
                point.n,
                unavailable.capability.c_str(),
                unavailable.reason.c_str()
            );
        }
    }

    std::fflush(stdout);
}

inline void complete(const BenchResult& result) {
    if (result.analysis.empty())
        return;

    for (const AnalysisResult& analysis : result.analysis) {
        switch (analysis.type) {
            case AnalysisType::Growth: {
                const AnalysisValueInfo x =
                    analysisValueInfo(result, analysis.x);
                const AnalysisValueInfo y =
                    analysisValueInfo(result, analysis.y);
                const std::string expression =
                    std::format("{}({})", y.name, x.name);

                if (analysis.bigO.complexity == Complexity::Unknown) {
                    std::fprintf(
                        stdout,
                        "Analysis  %-25s Unavailable\n",
                        expression.c_str()
                    );
                    break;
                }

                const std::string coefficient =
                    formatCoefficient(analysis.bigO.coefficient, y.unit);

                std::fprintf(
                    stdout,
                    "Analysis  %-25s %-7s k=%s error=%.2f%%\n",
                    expression.c_str(),
                    complexityName(analysis.bigO.complexity).data(),
                    coefficient.c_str(),
                    analysis.bigO.error * 100.0
                );
                break;
            }
            case AnalysisType::Correlation:
                break;
            }
    }

    std::fprintf(stdout, "\n");
    std::fflush(stdout);
}

}
