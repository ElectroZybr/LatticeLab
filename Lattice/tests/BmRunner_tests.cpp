#include <chrono>

#include <Lattice/Tools/BmRunner/Benchmarks.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {

namespace {

class GroupProbe : public Benchmarks::MetricCapability<"GroupProbe", GroupProbe> {
public:
    inline static constexpr auto value = defineMetric(
        "groupValue",
        Benchmarks::Unit::Count,
        Benchmarks::MetricFlags::Live
    );
    inline static constexpr auto Schema = defineSchema(value);

    Benchmarks::Metrics end() override {
        return {.schema = schema(), .values = {1.0}};
    }

    Benchmarks::Metrics result() override {
        return {.schema = schema(), .values = {1.0}};
    }
};

class LocalProbe : public Benchmarks::MetricCapability<"LocalProbe", LocalProbe> {
public:
    inline static constexpr auto value = defineMetric(
        "localValue",
        Benchmarks::Unit::Count,
        Benchmarks::MetricFlags::Live
    );
    inline static constexpr auto Schema = defineSchema(value);

    Benchmarks::Metrics end() override {
        return {.schema = schema(), .values = {1.0}};
    }

    Benchmarks::Metrics result() override {
        return {.schema = schema(), .values = {1.0}};
    }
};

struct BmRunnerFixture : Fixture {
    size_t value;

    explicit BmRunnerFixture(size_t n)
        : value(n) {}
};

bool hasMetric(
    const Benchmarks::BenchResult& result,
    std::string_view capabilityName,
    std::string_view metricName
) {
    for (const Benchmarks::PointResult& point : result.points) {
        for (const Benchmarks::StageResult& stage : point.stages) {
            for (const Benchmarks::CapabilityMetrics& capability : stage.capabilities) {
                if (capability.capability != capabilityName)
                    continue;

                for (const Benchmarks::MetricDesc& metric : capability.metrics.schema)
                    if (metric.name == metricName)
                        return true;
            }
        }
    }

    return false;
}

}

BENCH_GROUP(BmRunnerComposition) {
    bench.config.sizes = {4};
    bench.config.target = std::chrono::microseconds{0};
    bench.analysis.clear();

    bench.stages.clear();
    bench.stages.add<GroupProbe>().samples(1);
}

BENCH_GROUPED(BmRunnerComposition, BmRunnerInheritedStages, BmRunnerFixture) {
    bench.measure<BmRunnerFixture>(
        [](BmRunnerFixture& fixture) { return fixture.value; }
    );
}

BENCH_GROUPED(BmRunnerComposition, BmRunnerOverriddenStages, BmRunnerFixture) {
    bench.stages.clear();
    bench.stages.add<LocalProbe>().samples(1);

    bench.measure<BmRunnerFixture>(
        [](BmRunnerFixture& fixture) { return fixture.value + 1; }
    );
}

TEST(BmRunner_InheritsAndOverridesGroupStages, Fixture,
    "BmRunner должен наследовать stages группы и разрешать локальную замену.") {
    Benchmarks::BenchResult completed;

    Benchmarks::disableSampleCallback();
    Benchmarks::disableResultCallback();
    Benchmarks::setCompleteCallback(
        [&](const Benchmarks::BenchResult& result) {
            completed = result;
        }
    );

    Benchmarks::run("BmRunnerInheritedStages");
    REQUIRE(hasMetric(completed, "GroupProbe", "groupValue"));
    REQUIRE(!hasMetric(completed, "LocalProbe", "localValue"));

    Benchmarks::run("BmRunnerOverriddenStages");
    REQUIRE(!hasMetric(completed, "GroupProbe", "groupValue"));
    REQUIRE(hasMetric(completed, "LocalProbe", "localValue"));

    Benchmarks::disableCallbacks();
}

}
