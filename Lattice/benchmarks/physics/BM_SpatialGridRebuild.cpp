#include <benchmark/benchmark.h>

#include "SimulationFixture.h"

// @bench_meta {"id":"Fixture/SpatialGridRebuild","ru":"Перестройка SpatialGrid","group":"Симуляция/Сетка и соседи"}
BENCHMARK_DEFINE_F(Fixture, SpatialGridRebuild)(benchmark::State& state) {
    rebuildScene();

    auto& atoms = simulation_->atoms();
    auto& grid = simulation_->world().getGrid();

    for (auto _ : state) {
        grid.rebuild(atoms.xDataSpan(), atoms.yDataSpan(), atoms.zDataSpan());
        benchmark::DoNotOptimize(grid.stats().lastNonEmptyCellCount());
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

const auto spatialGridRebuildScene = Benchmarks::sceneFromEnv();
BENCHMARK_REGISTER_F(Fixture, SpatialGridRebuild)
    ->Arg(Benchmarks::atomsForScene(spatialGridRebuildScene, 5))
    ->Arg(Benchmarks::atomsForScene(spatialGridRebuildScene, 10));
