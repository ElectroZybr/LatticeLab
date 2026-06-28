#include <benchmark/benchmark.h>
#include "Fixture.h"

// @bench_meta {"id":"Fixture/NeighborListNeedRebuild","label":"Check NLneedsRebuild","group":"Simulation/Grid and Neighbors"}
BENCHMARK_DEFINE_F(Fixture, NeighborListNeedRebuild)(benchmark::State& state) {
    rebuildScene();
    warmupScene();

    for (auto _ : state) {
        const bool needsRebuild = simulation_->neighborList().needsRebuild(simulation_->atoms());
        benchmark::DoNotOptimize(static_cast<int>(needsRebuild));
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

BENCHMARK_REGISTER_F(Fixture, NeighborListNeedRebuild)
    ->Arg(5)
    ->Arg(10);
