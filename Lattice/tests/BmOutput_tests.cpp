#include <Lattice/Tools/BmRunner/Output.hpp>
#include <Lattice/Tools/BmRunner/Metrics/Time.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {

TEST(BmOutput_CompactsLargeCounters, Fixture) {
    using Benchmarks::Output::formatValue;

    REQUIRE(formatValue(1'780'894'539.0, Benchmarks::Unit::Count) == "1.78B");
    REQUIRE(formatValue(445'275'808.0, Benchmarks::Unit::Count) == "445M");
    REQUIRE(formatValue(17'895'377.0, Benchmarks::Unit::Count) == "17.9M");
    REQUIRE(formatValue(28'344.0, Benchmarks::Unit::Count) == "28.3K");
    REQUIRE(formatValue(897.0, Benchmarks::Unit::Count) == "897");
    REQUIRE(formatValue(74.2, Benchmarks::Unit::Count) == "74.2");
    REQUIRE(formatValue(0.4, Benchmarks::Unit::Count) == "0.400");
}

TEST(BmOutput_PreservesSmallRatios, Fixture) {
    using Benchmarks::Output::formatValue;

    REQUIRE(formatValue(0.041, Benchmarks::Unit::Ratio) == "0.041");
    REQUIRE(formatValue(0.000118, Benchmarks::Unit::Ratio) == "0.000118");
    REQUIRE(formatValue(0.000000118, Benchmarks::Unit::Ratio) == "0.000000118");
    REQUIRE(formatValue(0.000118, Benchmarks::Unit::Percent) == "0.000118%");
    REQUIRE(formatValue(0.0, Benchmarks::Unit::Ratio) == "0");
}

TEST(BmOutput_DescribesUnavailableMetricFromValueRef, Fixture) {
    const Benchmarks::BenchResult result{};
    const auto info = Benchmarks::Output::analysisValueInfo(
        result,
        Benchmarks::valueRef(Benchmarks::Time::median)
    );

    REQUIRE(info.name == "Time.median");
    REQUIRE(info.unit == Benchmarks::Unit::Nanoseconds);
}

TEST(BmOutput_FormatsBenchmarkLineAsStyledText, Fixture) {
    constexpr Benchmarks::MetricDesc schema[] = {
        {
            "median",
            Benchmarks::Unit::Nanoseconds,
            Benchmarks::MetricFlags::Live
        }
    };
    const Benchmarks::SampleResult sample{
        .name = "Case",
        .group = "Group",
        .stage = "Time",
        .n = 64,
        .sample = 2,
        .samples = 10,
        .overhead = 12.0,
        .capabilities = {
            {
                .capability = "Time",
                .metrics = {
                    .schema = schema,
                    .values = {1'500.0}
                }
            }
        }
    };

    const TextFormatter line = Benchmarks::Output::sampleLine(sample);

    REQUIRE(line.diagnostics().empty());
    REQUIRE(line.plain().find("Group/Case") != std::string::npos);
    REQUIRE(line.plain().find("N=64") != std::string::npos);
    REQUIRE(line.plain().find("sample= 2/10") != std::string::npos);
    REQUIRE(line.plain().find("median=1.50 us") != std::string::npos);
    REQUIRE(line.markup().find("<b>") != std::string::npos);
}

static_assert(Benchmarks::valueRef(Benchmarks::Time::median).index == 1);
static_assert(Benchmarks::Time::median.name == "median");
static_assert(
    Benchmarks::Time::median.unit ==
    Benchmarks::Unit::Nanoseconds
);

}
