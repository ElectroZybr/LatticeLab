#include <benchmark/benchmark.h>
#include "fixtures/SimulationFixture.h"

BENCHMARK_DEFINE_F(SimulationFixture, PredictAndSync)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        prepareForPredict();
        state.ResumeTiming();

        StepOps::predictAndSync(
            simulation_->atoms, simulation_->sim_box,
            Benchmarks::kDt, &VerletScheme::predict
        );
        benchmark::DoNotOptimize(simulation_->atoms.data());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, PredictAndSync)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);