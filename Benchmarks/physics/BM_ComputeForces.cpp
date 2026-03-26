#include <benchmark/benchmark.h>
#include "fixtures/SimulationFixture.h"

BENCHMARK_DEFINE_F(SimulationFixture, ComputeForcesNoNeighborList)(benchmark::State& state) {
    rebuildScene();

    for (auto _ : state) {
        StepOps::computeForces(
            simulation_->atomStorage, simulation_->sim_box,
            simulation_->forceField, nullptr, Benchmarks::kDt
        );
        benchmark::DoNotOptimize(simulation_->atomStorage.size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_DEFINE_F(SimulationFixture, ComputeForcesWithNeighborList)(benchmark::State& state) {
    rebuildScene();
    prepareNeighborList();

    for (auto _ : state) {
        StepOps::computeForces(
            simulation_->atomStorage, simulation_->sim_box,
            simulation_->forceField, &simulation_->neighborList, Benchmarks::kDt
        );
        benchmark::DoNotOptimize(simulation_->atomStorage.size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, ComputeForcesNoNeighborList)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);

BENCHMARK_REGISTER_F(SimulationFixture, ComputeForcesWithNeighborList)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
