#include <benchmark/benchmark.h>

#include "SimulationFixture.h"

// @bench_meta {"id":"SimulationFixture/NeighborListRebuildOnly","ru":"Перестройка NeighborList","group":"Симуляция/Сетка и соседи"}
BENCHMARK_DEFINE_F(SimulationFixture, NeighborListRebuildOnly)(benchmark::State& state) {
    rebuildScene();

    auto& atoms = simulation_->atoms();
    auto& grid = simulation_->world().getGrid();
    grid.rebuild(atoms.xDataSpan(), atoms.yDataSpan(), atoms.zDataSpan());

    for (auto _ : state) {
        simulation_->neighborList().build(simulation_->atoms(), simulation_->world());
        benchmark::DoNotOptimize(simulation_->neighborList().pairStorageSize());
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

const auto neighborListRebuildScene = Benchmarks::sceneFromEnv();
BENCHMARK_REGISTER_F(SimulationFixture, NeighborListRebuildOnly)
    ->Arg(Benchmarks::atomsForScene(neighborListRebuildScene, 5))
    ->Arg(Benchmarks::atomsForScene(neighborListRebuildScene, 10))
    ->Arg(Benchmarks::atomsForScene(neighborListRebuildScene, 25))
    ->Arg(Benchmarks::atomsForScene(neighborListRebuildScene, 47));
