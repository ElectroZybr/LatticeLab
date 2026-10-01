#include <cmath>
#include <vector>

#include <Lattice/Tools/BmRunner/Analysis.hpp>
#include <Lattice/Tools/BmRunner/Metrics/Perf.hpp>
#include <Lattice/Tools/BmRunner/Metrics/Time.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {

namespace {

using namespace Benchmarks;

constexpr MetricDesc timeSchema[] = {
    {"min", Unit::Nanoseconds},
    {"median", Unit::Nanoseconds},
    {"mean", Unit::Nanoseconds}
};

constexpr MetricDesc perfSchema[] = {
    {"cycles", Unit::Cycles},
    {"instructions", Unit::Count}
};

PointResult point(size_t n, double time, double instructions) {
    return {
        .n = n,
        .stages = {{
            .name = "Time|Perf",
            .capabilities = {
                {
                    .capability = "Time",
                    .metrics = {
                        .schema = timeSchema,
                        .values = {time, time, time}
                    }
                },
                {
                    .capability = "Perf",
                    .metrics = {
                        .schema = perfSchema,
                        .values = {instructions * 2.0, instructions}
                    }
                }
            }
        }}
    };
}

}

TEST(Analysis_CorrelationIsSymmetric, Fixture) {
    const std::vector<PointResult> points = {
        point(1, 10.0, 100.0),
        point(2, 20.0, 200.0),
        point(3, 40.0, 400.0),
        point(4, 80.0, 800.0)
    };

    Analysis forward;
    forward.correlation(
        Time::median,
        Perf::instructions
    );

    Analysis reverse;
    reverse.correlation(
        Perf::instructions,
        Time::median
    );

    const auto forwardResult = forward.run(points);
    const auto reverseResult = reverse.run(points);

    REQUIRE(forwardResult.size() == 1);
    REQUIRE(reverseResult.size() == 1);

    const AnalysisValue* forwardR = forwardResult.front().find("r");
    const AnalysisValue* reverseR = reverseResult.front().find("r");
    const AnalysisValue* forwardSamples = forwardResult.front().find("samples");
    const AnalysisValue* reverseSamples = reverseResult.front().find("samples");

    REQUIRE(forwardR != nullptr);
    REQUIRE(reverseR != nullptr);
    REQUIRE(forwardSamples != nullptr);
    REQUIRE(reverseSamples != nullptr);
    REQUIRE(forwardResult.front().values.size() == 2);
    REQUIRE(reverseResult.front().values.size() == 2);
    REQUIRE(forwardR->unit == Unit::Ratio);
    REQUIRE(forwardSamples->unit == Unit::Count);

    const double forwardCoefficient = std::get<double>(forwardR->value);
    const double reverseCoefficient = std::get<double>(reverseR->value);

    REQUIRE(std::get<uint64_t>(forwardSamples->value) == points.size());
    REQUIRE(std::get<uint64_t>(reverseSamples->value) == points.size());
    REQUIRE(forwardCoefficient == reverseCoefficient);
    REQUIRE(std::abs(forwardCoefficient - 1.0) < 1e-12);
}

TEST(Analysis_GrowthUsesGenericValues, Fixture) {
    const std::vector<PointResult> points = {
        point(1, 4.0, 100.0),
        point(2, 16.0, 200.0),
        point(4, 64.0, 400.0),
        point(8, 256.0, 800.0)
    };

    Analysis analysis;
    analysis.growth(N, Time::median);

    const auto results = analysis.run(points);

    REQUIRE(results.size() == 1);

    const AnalysisValue* complexity = results.front().find("");
    const AnalysisValue* coefficient = results.front().find("k");
    const AnalysisValue* error = results.front().find("error");

    REQUIRE(complexity != nullptr);
    REQUIRE(coefficient != nullptr);
    REQUIRE(error != nullptr);
    REQUIRE(std::get<std::string>(complexity->value) == "O(N^2)");
    REQUIRE(coefficient->unit == Unit::Nanoseconds);
    REQUIRE(error->unit == Unit::Percent);
    REQUIRE(std::abs(std::get<double>(coefficient->value) - 4.0) < 1e-12);
    REQUIRE(std::abs(std::get<double>(error->value)) < 1e-12);
}

}
