#include <benchmark/benchmark.h>
#include <vector>
#include "Fixture.h"

namespace {
    struct VelocityPostProcessData {
        std::vector<float> baseVx;
        std::vector<float> baseVy;
        std::vector<float> baseVz;
    };

    VelocityPostProcessData prepareVelocityPostProcessData(Lattice::Simulation& simulation) {
        VelocityPostProcessData data;
        const AtomStorage& atomStorage = simulation.atoms();
        const std::size_t mobileCount = atomStorage.mobileCount();

        data.baseVx.assign(atomStorage.vx().begin(), atomStorage.vx().begin() + static_cast<std::ptrdiff_t>(mobileCount));
        data.baseVy.assign(atomStorage.vy().begin(), atomStorage.vy().begin() + static_cast<std::ptrdiff_t>(mobileCount));
        data.baseVz.assign(atomStorage.vz().begin(), atomStorage.vz().begin() + static_cast<std::ptrdiff_t>(mobileCount));
        return data;
    }

    void restoreVelocities(AtomStorage& atomStorage, const VelocityPostProcessData& data) {
        std::copy(data.baseVx.begin(), data.baseVx.end(), atomStorage.vx().begin());
        std::copy(data.baseVy.begin(), data.baseVy.end(), atomStorage.vy().begin());
        std::copy(data.baseVz.begin(), data.baseVz.end(), atomStorage.vz().begin());
    }
}

// @bench_meta {"id":"Fixture/PostProcessVelocities","label":"Velocity Post-Process: clamp","group":"Simulation/Integrator"}
BENCHMARK_DEFINE_F(Fixture, PostProcessVelocities)(benchmark::State& state) {
    rebuildScene();
    warmupScene();
    VelocityPostProcessData data = prepareVelocityPostProcessData(*simulation_);

    for (auto _ : state) {
        state.PauseTiming();
        restoreVelocities(simulation_->atoms(), data);
        state.ResumeTiming();

        StepOps::postProcessVelocities(simulation_->atoms(), 1.0f);

        benchmark::DoNotOptimize(simulation_->atoms().vx()[0]);
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(Fixture, PostProcessVelocities)
    ->Arg(5)
    ->Arg(10);
