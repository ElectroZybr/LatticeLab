#include <benchmark/benchmark.h>
#include "fixtures/SimulationFixture.h"

BENCHMARK_DEFINE_F(SimulationFixture, FullStep)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        rebuildScene();
        state.ResumeTiming();

        simulation_->update(Benchmarks::kDt);
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, FullStep)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);