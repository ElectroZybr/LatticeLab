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

static_assert(Benchmarks::valueRef(Benchmarks::Time::median).index == 1);
static_assert(Benchmarks::Time::median.name == "median");
static_assert(
    Benchmarks::Time::median.unit ==
    Benchmarks::Unit::Nanoseconds
);

}
