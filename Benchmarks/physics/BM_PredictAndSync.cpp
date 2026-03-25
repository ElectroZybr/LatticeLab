#include <benchmark/benchmark.h>
#include "fixtures/SimulationFixture.h"

BENCHMARK_DEFINE_F(SimulationFixture, PredictAndSync)(benchmark::State& state) {
    prepareForPredict();

    for (auto _ : state) {
        StepOps::predictAndSync(
            simulation_->atomStorage, simulation_->sim_box,
            Benchmarks::kDt, &VerletScheme::predict
        );
        benchmark::DoNotOptimize(simulation_->atomStorage.size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, PredictAndSync)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
