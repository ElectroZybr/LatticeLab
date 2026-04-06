#include <benchmark/benchmark.h>

#include "Benchmarks/fixtures/SimulationFixture.h"

// @bench_meta {"id":"SimulationFixture/PredictAndSync","ru":"Predict + Sync","group":"Симуляция/Интегратор"}
BENCHMARK_DEFINE_F(SimulationFixture, PredictAndSync)(benchmark::State& state) {
    prepareForPredict();
    StepData stepData = makeStepData();

    for (auto _ : state) {
        StepData stepData = makeStepData();
        StepOps::predictAndSync(stepData, &VerletScheme::predict);
        benchmark::DoNotOptimize(simulation_->atoms().size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, PredictAndSync)->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
