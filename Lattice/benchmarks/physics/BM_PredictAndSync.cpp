#include <benchmark/benchmark.h>

#include "SimulationFixture.h"

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

const auto predictAndSyncScene = Benchmarks::sceneFromEnv();
BENCHMARK_REGISTER_F(SimulationFixture, PredictAndSync)
    ->Arg(Benchmarks::atomsForScene(predictAndSyncScene, 5))
    ->Arg(Benchmarks::atomsForScene(predictAndSyncScene, 10));
