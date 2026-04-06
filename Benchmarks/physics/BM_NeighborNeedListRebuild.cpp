#include <benchmark/benchmark.h>

#include "fixtures/SimulationFixture.h"

// @bench_meta {"id":"SimulationFixture/NeighborListNeedRebuild","ru":"Проверка NeighborList::needsRebuild","group":"Симуляция/Сетка и
// соседи"}
BENCHMARK_DEFINE_F(SimulationFixture, NeighborListNeedRebuild)(benchmark::State& state) {
    rebuildScene();

    for (auto _ : state) {
        simulation_->neighborList().needsRebuild(simulation_->atoms());
        benchmark::DoNotOptimize(simulation_->neighborList().pairStorageSize());
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, NeighborListNeedRebuild)->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
