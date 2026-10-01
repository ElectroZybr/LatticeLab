#pragma once

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <type_traits>

#include <Lattice/Tools/BmRunner/BenchTypes.hpp>
#include <Lattice/Tools/BmRunner/Analysis.hpp>
#include <Lattice/Tools/Logger.hpp>

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

inline TextFormatter formatMetrics(
    const Metrics& metrics,
    bool liveOnly = false
) {
    TextFormatter output;
    const size_t count =
        std::min(metrics.schema.size(), metrics.values.size());

    for (size_t i = 0; i < count; ++i) {
        const MetricDesc& desc = metrics.schema[i];

        if (liveOnly && !hasFlag(desc.flags, MetricFlags::Live))
            continue;

        const std::string value =
            formatValue(metrics.values[i], desc.unit);

        output += TextFormatter::format(
            "  <light>{}</><mut>={}</>",
            desc.name,
            value
        );
    }

    return output;
}

inline TextFormatter formatMetrics(
    const std::vector<CapabilityMetrics>& capabilities,
    bool liveOnly = false
) {
    TextFormatter output;

    for (const CapabilityMetrics& capability : capabilities)
        output += formatMetrics(capability.metrics, liveOnly);

    return output;
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

inline TextFormatter sampleLine(const SampleResult& sample) {
    const std::string name = fullName(sample.group, sample.name);
    const std::string overhead =
        formatValue(sample.overhead, Unit::Nanoseconds);

    TextFormatter output = TextFormatter::format(
        "<mut><light><b>{:<20}<//> <a2>N</>={:<7} <a>{:<12}</> "
        "ovhd={:<10} sample={:>2}</>",
        name,
        sample.n,
        sample.stage,
        overhead,
        sample.sample
    );

    if (sample.samples)
        output += TextFormatter::format("<mut>/{:<3}</>", sample.samples);

    output += formatMetrics(sample.capabilities, true);
    return output;
}

inline TextFormatter stageLine(
    const PointResult& point,
    const StageResult& stage
) {
    const std::string name = fullName(point.group, point.name);
    const std::string overhead =
        formatValue(stage.overhead, Unit::Nanoseconds);

    TextFormatter output = TextFormatter::format(
        "<mut><light><b>{:<20}<//> <a2>N</>={:<7} <a>{:<12}</> "
        "ovhd={:<10} <a>sample</>={:>2}</>",
        name,
        point.n,
        stage.name,
        overhead,
        stage.sample
    );

    if (stage.samples)
        output += TextFormatter::format("<mut>/{:<3}</>", stage.samples);

    output += formatMetrics(stage.capabilities, true);
    return output;
}

inline TextFormatter unavailableLine(
    const PointResult& point,
    const UnavailableCapability& unavailable
) {
    return TextFormatter::format(
        "<light><b>{:<16}<//> <a2>N</>={:<7} "
        "<wrn>{:<12} unavailable</>: {}",
        fullName(point.group, point.name),
        point.n,
        unavailable.capability,
        unavailable.reason
    );
}

inline void sample(const SampleResult& sample) {
    if (!hasLiveMetrics(sample.capabilities))
        return;

    LiveState& state = liveState();

    if (sample.sample == 1) {
        state.active = true;
    }

    LogSystem::writeConsole(
        "\r\033[2K" + sampleLine(sample).render()
    );
}

inline void result(const PointResult& point) {
    LiveState& state = liveState();

    if (state.active)
        LogSystem::writeConsole("\r\033[2K");

    state.active = false;

    for (const StageResult& stage : point.stages) {
        if (hasLiveMetrics(stage.capabilities))
            Logger::message(stageLine(point, stage));

        for (const UnavailableCapability& unavailable : stage.unavailable)
            Logger::message(unavailableLine(point, unavailable));
    }
}

inline TextFormatter analysisLine(
    const BenchResult& result,
    const AnalysisResult& analysis
) {
    const AnalysisValueInfo x = analysisValueInfo(result, analysis.x);
    const AnalysisValueInfo y = analysisValueInfo(result, analysis.y);
    std::string expression;

    switch (analysis.type) {
        case AnalysisType::Growth:
            expression = std::format("{}({})", y.name, x.name);
            break;
        case AnalysisType::Correlation:
            expression = std::format("corr({}, {})", x.name, y.name);
            break;
    }

    TextFormatter output = TextFormatter::format(
        "<a2><b>{:<10}<//> <light>{:<25}</>",
        "Analysis",
        expression
    );

    if (analysis.values.empty()) {
        output += TextFormatter(" <wrn>Unavailable</>");
        return output;
    }

    for (const AnalysisValue& value : analysis.values) {
        const std::string formatted = formatAnalysisValue(value);

        if (value.name.empty()) {
            output += TextFormatter::format(" <light>{:<7}</>", formatted);
        } else {
            output += TextFormatter::format(
                " <mut>{}</>=<light>{}</>",
                value.name,
                formatted
            );
        }
    }

    return output;
}

inline void complete(const BenchResult& result) {
    if (result.analysis.empty())
        return;

    for (const AnalysisResult& analysis : result.analysis)
        Logger::message(analysisLine(result, analysis));

    Logger::blank();
}

}
