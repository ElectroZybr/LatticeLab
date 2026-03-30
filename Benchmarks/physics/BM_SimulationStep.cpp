#include <benchmark/benchmark.h>
#include "fixtures/SimulationFixture.h"

// @bench_meta {"id":"SimulationFixture/FullStepWithNeighborList","ru":"Полный шаг с NeighborList","group":"Симуляция/Шаг симуляции"}
BENCHMARK_DEFINE_F(SimulationFixture, FullStepWithNeighborList)(benchmark::State& state) {
    constexpr int kWarmupSteps = 128;

    rebuildScene();
    for (int i = 0; i < kWarmupSteps; ++i) {
        simulation_->update(Benchmarks::kDt);
    }
    const size_t rebuildCountBefore = simulation_->neighborList.stats().rebuildCount();

    for (auto _ : state) {
        simulation_->update(Benchmarks::kDt);
        benchmark::ClobberMemory();
    }

    const size_t rebuildCountAfter = simulation_->neighborList.stats().rebuildCount();
    const size_t rebuildCount = rebuildCountAfter - rebuildCountBefore;
    const double iterCount = static_cast<double>(state.iterations());

    state.counters["nl_rebuild_count"] = static_cast<double>(rebuildCount);
    state.counters["nl_rebuilds_per_step"] = (iterCount > 0.0)
        ? static_cast<double>(rebuildCount) / iterCount
        : 0.0;
    state.counters["nl_avg_steps_between_rebuilds"] =
        static_cast<double>(simulation_->neighborList.stats().averageStepsBetweenRebuilds());
    state.counters["nl_steps_since_last_rebuild"] =
        static_cast<double>(simulation_->neighborList.stats().stepsSinceLastRebuild(simulation_->getSimStep()));

    setCounters(state);
}

// @bench_meta {"id":"SimulationFixture/FullStepNoNeighborList","ru":"Полный шаг без NeighborList","group":"Симуляция/Шаг симуляции"}
BENCHMARK_DEFINE_F(SimulationFixture, FullStepNoNeighborList)(benchmark::State& state) {
    constexpr int kWarmupSteps = 128;

    rebuildScene();
    simulation_->setNeighborListEnabled(false);
    for (int i = 0; i < kWarmupSteps; ++i) {
        simulation_->update(Benchmarks::kDt);
    }

    for (auto _ : state) {
        simulation_->update(Benchmarks::kDt);
        benchmark::ClobberMemory();
    }

    state.counters["nl_rebuild_count"] = 0.0;
    state.counters["nl_rebuilds_per_step"] = 0.0;
    state.counters["nl_avg_steps_between_rebuilds"] = 0.0;
    state.counters["nl_steps_since_last_rebuild"] =
        static_cast<double>(simulation_->neighborList.stats().stepsSinceLastRebuild(simulation_->getSimStep()));

    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, FullStepWithNeighborList)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);

BENCHMARK_REGISTER_F(SimulationFixture, FullStepNoNeighborList)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
