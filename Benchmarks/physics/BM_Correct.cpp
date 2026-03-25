#include <benchmark/benchmark.h>
#include "fixtures/SimulationFixture.h"

BENCHMARK_DEFINE_F(SimulationFixture, Correct)(benchmark::State& state) {
    prepareForCorrect();

    for (auto _ : state) {
        for (std::size_t atomIndex = 0; atomIndex < simulation_->atomStorage.size(); ++atomIndex) {
            if (!simulation_->atomStorage.isAtomFixed(atomIndex)) {
                VerletScheme::correct(simulation_->atomStorage, atomIndex, Benchmarks::kDt);
            }
        }
        benchmark::DoNotOptimize(simulation_->atomStorage.size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, Correct)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
