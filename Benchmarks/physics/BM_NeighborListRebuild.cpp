#include <benchmark/benchmark.h>

#include "fixtures/SimulationFixture.h"

BENCHMARK_DEFINE_F(SimulationFixture, NeighborListRebuild)(benchmark::State& state) {
    rebuildScene();

    for (auto _ : state) {
        simulation_->neighborList.build(simulation_->atomStorage, simulation_->sim_box);
        benchmark::DoNotOptimize(simulation_->neighborList.pairStorageSize());
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, NeighborListRebuild)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
