#include <benchmark/benchmark.h>
#include "fixtures/SimulationFixture.h"

BENCHMARK_DEFINE_F(SimulationFixture, ComputeForces)(benchmark::State& state) {
    rebuildScene();

    for (auto _ : state) {
        StepOps::computeForces(
            simulation_->atomStorage, simulation_->sim_box,
            simulation_->forceField, Benchmarks::kDt
        );
        benchmark::DoNotOptimize(simulation_->atomStorage.size());
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(SimulationFixture, ComputeForces)
    ->RangeMultiplier(8)->Range(Benchmarks::kAtomMin, Benchmarks::kAtomMax);
