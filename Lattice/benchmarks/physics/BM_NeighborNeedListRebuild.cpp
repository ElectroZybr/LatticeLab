#include <benchmark/benchmark.h>

#include "SimulationFixture.h"

// @bench_meta {"id":"SimulationFixture/NeighborListNeedRebuild","ru":"Проверка NeighborList::needsRebuild","group":"Симуляция/Сетка и соседи"}
BENCHMARK_DEFINE_F(SimulationFixture, NeighborListNeedRebuild)(benchmark::State& state) {
    rebuildScene();

    for (auto _ : state) {
        simulation_->neighborList().needsRebuild(simulation_->atoms());
        benchmark::DoNotOptimize(simulation_->neighborList().pairStorageSize());
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

const auto neighborNeedListScene = Benchmarks::sceneFromEnv();
BENCHMARK_REGISTER_F(SimulationFixture, NeighborListNeedRebuild)
    ->Arg(Benchmarks::atomsForScene(neighborNeedListScene, 5))
    ->Arg(Benchmarks::atomsForScene(neighborNeedListScene, 10));
