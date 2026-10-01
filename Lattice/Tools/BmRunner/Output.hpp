#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <format>
#include <string>
#include <type_traits>

#include <Lattice/Tools/BmRunner/BenchTypes.hpp>
#include "Lattice/Tools/BmRunner/Analysis.hpp"
#include <Lattice/Tools/SysInfo/SystemInfo.hpp>

namespace Lattice::Benchmarks::Output {

/**
 @file Output.hpp
 @brief Стандартный консольный вывод результатов бенчмарков.

 Форматирует промежуточные и итоговые результаты измерений
 для вывода в стандартный поток.
*/

inline std::string formatRatio(double value);

inline std::string formatCompactCount(double value) {
    struct Scale {
        double threshold;
        std::string_view suffix;
    };

    constexpr Scale scales[] = {
        {1'000'000'000'000.0, "T"},
        {1'000'000'000.0, "B"},
        {1'000'000.0, "M"},
        {1'000.0, "K"}
    };

    const double absolute = std::abs(value);

    for (const Scale& scale : scales) {
        if (absolute < scale.threshold)
            continue;

        const double scaled = value / scale.threshold;
        const double scaledAbsolute = std::abs(scaled);
        const int precision =
            scaledAbsolute >= 100.0 ? 0 :
            scaledAbsolute >= 10.0 ? 1 : 2;

        return std::format(
            "{:.{}f}{}",
            scaled,
            precision,
            scale.suffix
        );
    }

    if (value == std::trunc(value))
        return std::format("{:.0f}", value);

    if (absolute >= 10.0)
        return std::format("{:.1f}", value);

    if (absolute >= 1.0)
        return std::format("{:.2f}", value);

    return formatRatio(value);
}

inline std::string formatRatio(double value) {
    const double absolute = std::abs(value);

    if (absolute == 0.0)
        return "0";

    if (absolute >= 0.001)
        return std::format("{:.3f}", value);

    if (absolute < 1e-9)
        return std::format("{:.3e}", value);

    const int precision = std::clamp(
        static_cast<int>(std::ceil(-std::log10(absolute))) + 2,
        4,
        9
    );

    return std::format("{:.{}f}", value, precision);
}

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

        case Unit::Percent: return std::format("{}%", formatRatio(value));
        case Unit::Ratio: return formatRatio(value);
        case Unit::Count:
        case Unit::Cycles: return formatCompactCount(value);
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
        return {
            ref.name.empty()
                ? std::format("parameter[{}]", ref.index)
                : std::string(ref.name),
            ref.unit
        };
    }

    if (!ref.name.empty()) {
        return {
            std::format("{}.{}", ref.capability, ref.name),
            ref.unit
        };
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

    const std::string formatted = formatRatio(value);

    return suffix.empty()
        ? formatted
        : std::format("{} {}", formatted, suffix);
}

inline std::string formatAnalysisValue(const AnalysisValue& value) {
    return std::visit(
        [&](const auto& data) -> std::string {
            using T = std::decay_t<decltype(data)>;

            if constexpr (std::is_same_v<T, std::string>) {
                return data;
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                return std::format("{}", data);
            } else {
                if (value.unit == Unit::Percent)
                    return std::format("{:.2f}%", data);

                return formatCoefficient(data, value.unit);
            }
        },
        value.value
    );
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
        const AnalysisValueInfo x =
            analysisValueInfo(result, analysis.x);
        const AnalysisValueInfo y =
            analysisValueInfo(result, analysis.y);
        std::string expression;

        switch (analysis.type) {
            case AnalysisType::Growth:
                expression = std::format("{}({})", y.name, x.name);
                break;
            case AnalysisType::Correlation:
                expression = std::format("corr({}, {})", x.name, y.name);
                break;
        }

        std::fprintf(stdout, "Analysis  %-25s", expression.c_str());

        if (analysis.values.empty()) {
            std::fprintf(stdout, " Unavailable\n");
            continue;
        }

        for (const AnalysisValue& value : analysis.values) {
            const std::string formatted = formatAnalysisValue(value);

            if (value.name.empty()) {
                std::fprintf(
                    stdout,
                    " %-7s",
                    formatted.c_str()
                );
            } else {
                std::fprintf(
                    stdout,
                    " %s=%s",
                    value.name.c_str(),
                    formatted.c_str()
                );
            }
        }

        std::fprintf(stdout, "\n");
    }

    std::fprintf(stdout, "\n");
    std::fflush(stdout);
}

inline std::string formatFrequency(uint64_t hz) {
    if (hz >= 1'000'000'000ull) return std::format("{:.2f} GHz", hz / 1e9);
    if (hz >= 1'000'000ull) return std::format("{:.2f} MHz", hz / 1e6);
    return std::format("{} Hz", hz);
}

inline std::string formatCpuList(const std::vector<uint32_t>& cpus) {
    std::string result;

    for (size_t i = 0; i < cpus.size(); ++i) {
        if (i) result += ",";
        result += std::format("{}", cpus[i]);
    }

    return result;
}

inline std::string formatMachineInfo(const SystemInfo::MachineInfo& info) {
    std::string out;

    out += std::format("System    {}\n", info.os);
    out += std::format("Memory    {} total, {} available\n",
        formatValue(static_cast<double>(info.memory.totalBytes), Unit::Bytes),
        formatValue(static_cast<double>(info.memory.availableBytes), Unit::Bytes));

    for (const auto& node : info.memory.nodes)
        out += std::format("  NUMA {}  {}  CPUs={}\n",
            node.id,
            formatValue(static_cast<double>(node.totalBytes), Unit::Bytes),
            formatCpuList(node.logicalCpus));

    for (const auto& cpu : info.processors) {
        size_t threads = 0;
        for (const auto& core : cpu.cores)
            threads += core.logicalCpus.size();

        out += std::format("CPU {}     {}  {} cores / {} threads  {}-{}\n",
            cpu.id, cpu.name, cpu.cores.size(), threads,
            formatFrequency(cpu.minFrequencyHz),
            formatFrequency(cpu.maxFrequencyHz));

        for (const auto& cache : cpu.caches) {
            std::string_view type =
                cache.type == SystemInfo::CacheType::Data ? "D" :
                cache.type == SystemInfo::CacheType::Instruction ? "I" : "";

            out += std::format("  L{}{}     {}  {}-way  line={} B  CPUs={}\n",
                cache.level,
                type,
                formatValue(static_cast<double>(cache.sizeBytes), Unit::Bytes),
                cache.ways,
                cache.lineSizeBytes,
                formatCpuList(cache.sharedLogicalCpus));
        }
    }

    for (size_t i = 0; i < info.gpus.size(); ++i) {
        const auto& gpu = info.gpus[i];

        out += std::format("GPU {}     {}  driver={}  pci={}",
            i, gpu.name, gpu.driver, gpu.pciAddress);

        if (gpu.vramBytes)
            out += std::format("  VRAM={}",
                formatValue(static_cast<double>(gpu.vramBytes), Unit::Bytes));

        out += '\n';
    }

    return out;
}

}
