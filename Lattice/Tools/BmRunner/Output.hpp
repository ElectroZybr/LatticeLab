#pragma once

#include <algorithm>
#include <cstdio>
#include <format>
#include <string>

#include <Lattice/Tools/BmRunner/BenchTypes.hpp>

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
    const std::vector<CapabilityMetrics>& capabilities,
    bool liveOnly = false
) {
    for (const CapabilityMetrics& capability : capabilities) {
        const Metrics& metrics = capability.metrics;
        const size_t count = std::min(metrics.schema.size(), metrics.values.size());

        for (size_t i = 0; i < count; ++i) {
            const MetricDesc& desc = metrics.schema[i];

            if (liveOnly && !hasFlag(desc.flags, MetricFlags::Live))
                continue;

            const std::string value = formatValue(metrics.values[i], desc.unit);

            std::fprintf(
                out,
                "  %.*s=%s",
                static_cast<int>(desc.name.size()),
                desc.name.data(),
                value.c_str()
            );
        }
    }
}

inline void sample(const SampleResult& sample) {
    const std::string name = fullName(sample.group, sample.name);

    std::fprintf(
        stdout,
        "\r\033[2K%-16s N=%-7zu %-12.*s ",
        name.c_str(),
        sample.n,
        static_cast<int>(sample.stage.size()),
        sample.stage.data()
    );

    if (sample.samples)
        std::fprintf(stdout, "%2zu/%-2zu", sample.sample, sample.samples);
    else
        std::fprintf(stdout, "%2zu", sample.sample);

    printMetrics(stdout, sample.capabilities, true);
    std::fflush(stdout);
}

inline void result(const PointResult& result) {
    const std::string name = fullName(result.group, result.name);

    std::fprintf(stdout, "\r\033[2K%s  N=%zu\n", name.c_str(), result.n);

    for (const CapabilityMetrics& capability : result.capabilities) {
        std::fprintf(
            stdout,
            "  %-8.*s",
            static_cast<int>(capability.capability.size()),
            capability.capability.data()
        );

        const Metrics& metrics = capability.metrics;
        const size_t count = std::min(metrics.schema.size(), metrics.values.size());

        for (size_t i = 0; i < count; ++i) {
            const MetricDesc& desc = metrics.schema[i];
            const std::string value = formatValue(metrics.values[i], desc.unit);

            std::fprintf(
                stdout,
                "  %.*s=%s",
                static_cast<int>(desc.name.size()),
                desc.name.data(),
                value.c_str()
            );
        }

        std::fprintf(stdout, "\n");
    }
}

inline void complete(const BenchResult&) {}

}