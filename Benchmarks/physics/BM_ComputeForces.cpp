#include <benchmark/benchmark.h>

#include "fixtures/SimulationFixture.h"

// @bench_meta {"id":"SimulationFixture/ComputeForcesWithNeighborList","ru":"Расчет сил с NeighborList","group":"Симуляция/Силы"}
BENCHMARK_DEFINE_F(SimulationFixture, ComputeForcesWithNeighborList)(benchmark::State& state) {
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

BENCHMARK_REGISTER_F(SimulationFixture, ComputeForcesWithNeighborList)
    ->RangeMultiplier(8)
    ->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
