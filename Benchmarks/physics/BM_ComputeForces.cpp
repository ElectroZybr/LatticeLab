#include <benchmark/benchmark.h>
#include "fixtures/SimulationFixture.h"

// @bench_meta {"id":"SimulationFixture/ComputeForcesNoNeighborList","ru":"Расчет сил без NeighborList","group":"Симуляция/Силы"}
BENCHMARK_DEFINE_F(SimulationFixture, ComputeForcesNoNeighborList)(benchmark::State& state) {
    rebuildScene();

    for (auto _ : state) {
        StepOps::computeForces(
            simulation_->atoms(), simulation_->bonds(), simulation_->box(),
            simulation_->forceField(), nullptr, simulation_->isBondFormationEnabled(), Benchmarks::kDt
        );
        benchmark::DoNotOptimize(simulation_->atoms().size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

// @bench_meta {"id":"SimulationFixture/ComputeForcesWithNeighborList","ru":"Расчет сил с NeighborList","group":"Симуляция/Силы"}
BENCHMARK_DEFINE_F(SimulationFixture, ComputeForcesWithNeighborList)(benchmark::State& state) {
    rebuildScene();
    prepareNeighborList();

    for (auto _ : state) {
        StepOps::computeForces(
            simulation_->atoms(), simulation_->bonds(), simulation_->box(),
            simulation_->forceField(), &simulation_->neighborList(), simulation_->isBondFormationEnabled(), Benchmarks::kDt
        );
        benchmark::DoNotOptimize(simulation_->atoms().size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, ComputeForcesNoNeighborList)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);

BENCHMARK_REGISTER_F(SimulationFixture, ComputeForcesWithNeighborList)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
