#include <benchmark/benchmark.h>
#include "fixtures/SimulationFixture.h"

BENCHMARK_DEFINE_F(SimulationFixture, Correct)(benchmark::State& state) {
    prepareForCorrect();

    for (auto _ : state) {
        VerletScheme::correct(simulation_->atomStorage, Benchmarks::kDt);
        benchmark::DoNotOptimize(simulation_->atomStorage.size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, Correct)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
