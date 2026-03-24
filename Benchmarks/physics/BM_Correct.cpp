#include <benchmark/benchmark.h>
#include "fixtures/SimulationFixture.h"

BENCHMARK_DEFINE_F(SimulationFixture, Correct)(benchmark::State& state) {
    prepareForCorrect();

    for (auto _ : state) {
        for (Atom& atom : simulation_->atoms) {
            if (!atom.isFixed)
                VerletScheme::correct(atom, Benchmarks::kDt);
        }
        benchmark::DoNotOptimize(simulation_->atoms.data());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, Correct)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
