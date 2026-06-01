#include <benchmark/benchmark.h>

#include "SimulationFixture.h"

// @bench_meta {"id":"Fixture/ComputeForces","ru":"Расчет сил","group":"Симуляция/Силы"}
BENCHMARK_DEFINE_F(Fixture, ComputeForces)(benchmark::State& state) {
    rebuildScene();
    prepareNeighborList();
    StepData stepData = makeStepData();

    for (auto _ : state) {
        StepOps::computeForces(stepData);
        benchmark::DoNotOptimize(simulation_->atoms().size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

const auto computeForcesScene = Benchmarks::sceneFromEnv();
BENCHMARK_REGISTER_F(Fixture, ComputeForces)
    ->Arg(Benchmarks::atomsForScene(computeForcesScene, 5))
    ->Arg(Benchmarks::atomsForScene(computeForcesScene, 10))
    ->Arg(Benchmarks::atomsForScene(computeForcesScene, 25))
    ->Arg(Benchmarks::atomsForScene(computeForcesScene, 47));


// @bench_meta {"id":"Fixture/ComputePairInteractions","ru":"Расчет PairInteraction","group":"Симуляция/Силы"}
BENCHMARK_DEFINE_F(Fixture, ComputePairInteractions)(benchmark::State& state) {
    rebuildScene();
    prepareNeighborList();

    for (auto _ : state) {
        simulation_->forceField().computePairInteractions(simulation_->world());
        benchmark::DoNotOptimize(simulation_->atoms().size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(Fixture, ComputePairInteractions)
    ->Arg(Benchmarks::atomsForScene(computeForcesScene, 5))
    ->Arg(Benchmarks::atomsForScene(computeForcesScene, 10))
    ->Arg(Benchmarks::atomsForScene(computeForcesScene, 25))
    ->Arg(Benchmarks::atomsForScene(computeForcesScene, 47));
