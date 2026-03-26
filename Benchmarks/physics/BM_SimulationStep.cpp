#include <benchmark/benchmark.h>
#include "fixtures/SimulationFixture.h"

BENCHMARK_DEFINE_F(SimulationFixture, FullStep)(benchmark::State& state) {
    rebuildScene();
    const std::size_t rebuildCountBefore = simulation_->neighborListRebuildCount();

    for (auto _ : state) {
        simulation_->update(Benchmarks::kDt);
        benchmark::ClobberMemory();
    }

    const std::size_t rebuildCountAfter = simulation_->neighborListRebuildCount();
    const std::size_t rebuildCount = rebuildCountAfter - rebuildCountBefore;
    const double iterCount = static_cast<double>(state.iterations());

    state.counters["nl_rebuild_count"] = static_cast<double>(rebuildCount);
    state.counters["nl_rebuilds_per_step"] = (iterCount > 0.0)
        ? static_cast<double>(rebuildCount) / iterCount
        : 0.0;
    state.counters["nl_avg_steps_between_rebuilds"] =
        static_cast<double>(simulation_->averageStepsPerNeighborListRebuild());
    state.counters["nl_steps_since_last_rebuild"] =
        static_cast<double>(simulation_->stepsSinceNeighborListRebuild());

    setCounters(state);
}

BENCHMARK_DEFINE_F(SimulationFixture, FullStepNoNeighborList)(benchmark::State& state) {
    rebuildScene();
    simulation_->setNeighborListEnabled(false);

    for (auto _ : state) {
        simulation_->update(Benchmarks::kDt);
        benchmark::ClobberMemory();
    }

    state.counters["nl_rebuild_count"] = 0.0;
    state.counters["nl_rebuilds_per_step"] = 0.0;
    state.counters["nl_avg_steps_between_rebuilds"] = 0.0;
    state.counters["nl_steps_since_last_rebuild"] =
        static_cast<double>(simulation_->stepsSinceNeighborListRebuild());

    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, FullStep)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);

BENCHMARK_REGISTER_F(SimulationFixture, FullStepNoNeighborList)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
