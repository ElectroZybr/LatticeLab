#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <Lattice/Tools/BmRunner/BenchTypes.hpp>


namespace Lattice::Benchmarks {

enum class Parameter : uint8_t {
    N
};

inline constexpr Parameter N = Parameter::N;

constexpr ValueRef valueRef(Parameter parameter) {
    return {
        .source = ValueSource::Parameter,
        .capability = {},
        .name = "N",
        .unit = Unit::None,
        .index = static_cast<size_t>(parameter)
    };
}

constexpr ValueRef valueRef(ValueRef ref) noexcept {
    return ref;
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

using AnalysisValueData = std::variant<double, uint64_t, std::string>;

struct AnalysisValue {
    std::string name;
    AnalysisValueData value;
    Unit unit = Unit::None;
};

struct AnalysisResult {
    AnalysisType type;
    ValueRef x;
    ValueRef y;
    std::vector<AnalysisValue> values;

    const AnalysisValue* find(std::string_view name) const noexcept {
        for (const AnalysisValue& value : values)
            if (value.name == name)
                return &value;

        return nullptr;
    }
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

    template<typename X, typename Y>
    Analysis& correlation(X x, Y y) {
        requests_.push_back({
            .type = AnalysisType::Correlation,
            .x = valueRef(x),
            .y = valueRef(y)
        });

        return *this;
    }

    std::vector<AnalysisResult> run(std::span<const PointResult> points) const;
    void clear() { requests_.clear(); }

private:
    static double resolve(const PointResult& point, ValueRef ref);
    static Unit resolveUnit(
        std::span<const PointResult> points,
        ValueRef ref
    );
    static std::vector<AnalysisValue> analyzeGrowth(
        std::span<const PointResult> points,
        const AnalysisRequest& request
    );
    static std::vector<AnalysisValue> analyzeCorrelation(
        std::span<const PointResult> points,
        const AnalysisRequest& request
    );
};

}
