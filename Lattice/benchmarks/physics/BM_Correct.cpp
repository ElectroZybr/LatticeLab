#include <benchmark/benchmark.h>

#include "SimulationFixture.h"

// @bench_meta {"id":"SimulationFixture/Correct","ru":"Correct","group":"Симуляция/Интегратор"}
BENCHMARK_DEFINE_F(SimulationFixture, Correct)(benchmark::State& state) {
    prepareForCorrect();

    for (auto _ : state) {
        VerletScheme::correct(simulation_->atoms(), 1.0f, Benchmarks::kDt);
        benchmark::DoNotOptimize(simulation_->atoms().size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

const auto correctScene = Benchmarks::sceneFromEnv();
BENCHMARK_REGISTER_F(SimulationFixture, Correct)
    ->Arg(Benchmarks::atomsForScene(correctScene, 5))
    ->Arg(Benchmarks::atomsForScene(correctScene, 10))
    ->Arg(Benchmarks::atomsForScene(correctScene, 22))
    ->Arg(Benchmarks::atomsForScene(correctScene, 47));
