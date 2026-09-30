#include <chrono>
#include <vector>

#include <Lattice/Tools/BmRunner/Benchmarks.hpp>
#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {

BENCH_GROUP(BmRunnerComposition) {
    bench.config.sizes = {4};
    bench.config.samples = 2;
    bench.config.target = std::chrono::microseconds{0};

    bench.stages.clear();
    bench.stages.add<Memory>();
    bench.stages.add<Allocations>();
}

BENCH(BmRunnerComposition, BmRunnerInheritedStages) {
    bench.measure(
        [](size_t n) { return n; },
        [](size_t& n) { return std::vector<int>(n); }
    );
}

BENCH(BmRunnerComposition, BmRunnerOverriddenStages) {
    bench.stages.clear();
    bench.stages.add<Time>();

    bench.measure(
        [](size_t n) { return n; },
        [](size_t& n) { return n + 1; }
    );
}

TEST(BmRunner_InheritsAndOverridesGroupStages, RuntimeFixture,
    "BmRunner должен наследовать stages группы и разрешать локальную замену.") {
    auto& benchmarks = Benchmarks::instance();
    bool sawMemoryProgress = false;
    bool sawAllocationsProgress = false;
    bool sawTimeProgress = false;

    benchmarks.setProgressCallback([&](const Progress& progress) {
        sawMemoryProgress |= progress.stage == "Memory" &&
            !progress.metrics.empty() &&
            progress.metrics.front().name == "memory";
        sawAllocationsProgress |= progress.stage == "Allocations" &&
            !progress.metrics.empty() &&
            progress.metrics.front().name == "allocations";
        sawTimeProgress |= progress.stage == "Time" &&
            !progress.metrics.empty() &&
            progress.metrics.front().name == "time";
    });

    const auto inherited = benchmarks.run("BmRunnerInheritedStages");
    REQUIRE(inherited.results.size() == 1);
    REQUIRE(inherited.results.front().find("N") != nullptr);
    REQUIRE(inherited.results.front().find("memory") != nullptr);
    REQUIRE(inherited.results.front().find("allocations") != nullptr);
    REQUIRE(inherited.results.front().find("time.mean") == nullptr);
    REQUIRE(sawMemoryProgress);
    REQUIRE(sawAllocationsProgress);

    const auto overridden = benchmarks.run("BmRunnerOverriddenStages");
    REQUIRE(overridden.results.size() == 1);
    REQUIRE(overridden.results.front().find("time.mean") != nullptr);
    REQUIRE(overridden.results.front().find("memory") == nullptr);
    REQUIRE(overridden.results.front().find("allocations") == nullptr);
    REQUIRE(sawTimeProgress);

    benchmarks.resetProgressCallback();
}

}
