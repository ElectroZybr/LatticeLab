#include <algorithm>
#include <cmath>
#include <limits>

#include <Lattice/Tools/BmRunner/Analysis.hpp>
#include <Lattice/Tools/BmRunner/BigO.hpp>

namespace Lattice::Benchmarks {

std::vector<AnalysisResult> Analysis::run(
    std::span<const PointResult> points
) const {
    std::vector<AnalysisResult> results;
    results.reserve(requests_.size());

    for (const AnalysisRequest& request : requests_) {
        switch (request.type) {
            case AnalysisType::Growth:
                results.push_back({
                    .type = request.type,
                    .x = request.x,
                    .y = request.y,
                    .values = analyzeGrowth(points, request)
                });
                break;
            case AnalysisType::Correlation:
                results.push_back({
                    .type = request.type,
                    .x = request.x,
                    .y = request.y,
                    .values = analyzeCorrelation(points, request)
                });
                break;
            }
    }

    return results;
}

double Analysis::resolve(
    const PointResult& point,
    ValueRef ref
) {
    if (ref.source == ValueSource::Parameter) {
        switch (ref.index) {
            case static_cast<size_t>(Parameter::N):
                return static_cast<double>(point.n);
        }

        return std::numeric_limits<double>::quiet_NaN();
    }

    const CapabilityMetrics* found = nullptr;

    for (const StageResult& stage : point.stages) {
        for (const CapabilityMetrics& capability : stage.capabilities) {
            if (capability.capability != ref.capability)
                continue;

            if (found)
                return std::numeric_limits<double>::quiet_NaN();

            found = &capability;
        }
    }

    if (!found)
        return std::numeric_limits<double>::quiet_NaN();

    if (ref.index >= found->metrics.values.size())
        return std::numeric_limits<double>::quiet_NaN();

    return found->metrics.values[ref.index];
}

Unit Analysis::resolveUnit(
    std::span<const PointResult> points,
    ValueRef ref
) {
    if (!ref.name.empty())
        return ref.unit;

    if (ref.source == ValueSource::Parameter)
        return Unit::None;

    for (const PointResult& point : points) {
        for (const StageResult& stage : point.stages) {
            for (const CapabilityMetrics& capability : stage.capabilities) {
                if (
                    capability.capability == ref.capability &&
                    ref.index < capability.metrics.schema.size()
                ) {
                    return capability.metrics.schema[ref.index].unit;
                }
            }
        }
    }

    return Unit::None;
}

std::vector<AnalysisValue> Analysis::analyzeGrowth(
    std::span<const PointResult> points,
    const AnalysisRequest& request
) {
    std::vector<ComplexityPoint> values;
    values.reserve(points.size());

    for (const PointResult& point : points) {
        const double x = resolve(point, request.x);
        const double y = resolve(point, request.y);

        if (!std::isfinite(x) || !std::isfinite(y))
            continue;

        values.push_back({
            .x = x,
            .y = y
        });
    }

    const BigOResult result = analyze(values);

    if (result.complexity == Complexity::Unknown)
        return {};

    return {
        {
            .name = {},
            .value = std::string(complexityName(result.complexity))
        },
        {
            .name = "k",
            .value = result.coefficient,
            .unit = resolveUnit(points, request.y)
        },
        {
            .name = "error",
            .value = result.error * 100.0,
            .unit = Unit::Percent
        }
    };
}

std::vector<AnalysisValue> Analysis::analyzeCorrelation(
    std::span<const PointResult> points,
    const AnalysisRequest& request
) {
    std::vector<ComplexityPoint> values;
    values.reserve(points.size());

    for (const PointResult& point : points) {
        const double x = resolve(point, request.x);
        const double y = resolve(point, request.y);

        if (!std::isfinite(x) || !std::isfinite(y))
            continue;

        values.push_back({
            .x = x,
            .y = y
        });
    }

    if (values.size() < 2)
        return {};

    double meanX = 0;
    double meanY = 0;

    for (const ComplexityPoint& value : values) {
        meanX += value.x;
        meanY += value.y;
    }

    const double count = static_cast<double>(values.size());
    meanX /= count;
    meanY /= count;

    double covariance = 0;
    double varianceX = 0;
    double varianceY = 0;

    for (const ComplexityPoint& value : values) {
        const double x = value.x - meanX;
        const double y = value.y - meanY;

        covariance += x * y;
        varianceX += x * x;
        varianceY += y * y;
    }

    const double divisor = std::sqrt(varianceX * varianceY);

    if (!(divisor > 0.0) || !std::isfinite(divisor))
        return {};

    return {
        {
            .name = "r",
            .value = std::clamp(covariance / divisor, -1.0, 1.0),
            .unit = Unit::Ratio
        },
        {
            .name = "samples",
            .value = static_cast<uint64_t>(values.size()),
            .unit = Unit::Count
        }
    };
}

}
