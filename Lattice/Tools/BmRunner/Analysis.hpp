#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include <Lattice/Tools/BmRunner/BenchTypes.hpp>
#include <Lattice/Tools/BmRunner/BigO.hpp>


namespace Lattice::Benchmarks {

enum class ValueSource : uint8_t {
    Parameter,
    Metric
};

struct ValueRef {
    ValueSource source;
    std::string_view capability;
    size_t index;
};

enum Metric : uint8_t {
    N,
    _count
};

constexpr ValueRef valueRef(Metric metric) {
    return {
        .source = ValueSource::Parameter,
        .capability = {},
        .index = static_cast<size_t>(metric)
    };
}

enum class AnalysisType : uint8_t {
    Growth,
    Correlation
};

struct AnalysisRequest {
    AnalysisType type;
    ValueRef x;
    ValueRef y;
};

struct AnalysisResult {
    AnalysisType type;
    ValueRef x;
    ValueRef y;
    BigOResult bigO;
};

struct BenchResult {
    std::string_view name;
    std::string_view group;
    std::vector<PointResult> points;
    std::vector<AnalysisResult> analysis;
};

class Analysis {
    std::vector<AnalysisRequest> requests_;

public:
    template<typename X, typename Y>
    Analysis& growth(X x, Y y) {
        requests_.push_back({
            .type = AnalysisType::Growth,
            .x = valueRef(x),
            .y = valueRef(y)
        });

        return *this;
    }

    std::vector<AnalysisResult> run(std::span<const PointResult> points) const;
    void clear() { requests_.clear(); }

private:
    static double resolve(const PointResult& point, ValueRef ref);
    static BigOResult analyzeGrowth(std::span<const PointResult> points, const AnalysisRequest& request);
};

}