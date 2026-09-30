#include <cmath>

#include <Lattice/Tools/BmRunner/Analysis.hpp>

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
                    .bigO = analyzeGrowth(points, request)
                });
                break;
            case AnalysisType::Correlation:
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
            case N:
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

BigOResult Analysis::analyzeGrowth(
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

    return analyze(values);
}

}