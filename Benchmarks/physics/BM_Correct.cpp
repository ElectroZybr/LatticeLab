#include <benchmark/benchmark.h>
#include "fixtures/SimulationFixture.h"

// @bench_meta {"id":"SimulationFixture/Correct","ru":"Correct","group":"Симуляция/Интегратор"}
BENCHMARK_DEFINE_F(SimulationFixture, Correct)(benchmark::State& state) {
    prepareForCorrect();

    for (auto _ : state) {
        VerletScheme::correct(simulation_->atomStorage, 1.0f, Benchmarks::kDt);
        benchmark::DoNotOptimize(simulation_->atomStorage.size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, Correct)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax)
    ->Args({10648})   // 22^3
    ->Args({103823}); // 47^3