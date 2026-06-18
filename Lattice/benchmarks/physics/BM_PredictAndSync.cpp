#include <benchmark/benchmark.h>
#include "Fixture.h"

// @bench_meta {"id":"Fixture/PredictAndSync","label":"Predict + Sync","group":"Simulation/Integrator"}
BENCHMARK_DEFINE_F(Fixture, PredictAndSync)(benchmark::State& state) {
    prepareForPredict();
    StepContext stepData = makeStepData();

    for (auto _ : state) {
        StepContext stepData = makeStepData();
        StepOps::predictAndSync(stepData, &Verlet::predict);
        benchmark::DoNotOptimize(simulation_->atoms().size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(Fixture, PredictAndSync)
    ->Arg(5)
    ->Arg(10);
