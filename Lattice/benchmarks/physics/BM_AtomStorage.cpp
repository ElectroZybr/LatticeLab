#include <benchmark/benchmark.h>

#include "Fixture.h"
#include "Lattice/Engine/restrict.h"

// @bench_meta {"id":"Fixture/AtomStorageReadXYZ","label":"AtomStorage: read XYZ","group":"Simulation/Data Layout"}
BENCHMARK_DEFINE_F(Fixture, AtomStorageReadXYZ)(benchmark::State& state) {
    rebuildScene();
    warmupScene();

    const AtomStorage& atoms = simulation_->atoms();
    const size_t n = atoms.mobileCount();

    for (auto _ : state) {
        const float* RESTRICT xs = atoms.x().data();
        const float* RESTRICT ys = atoms.y().data();
        const float* RESTRICT zs = atoms.z().data();
        float sum = 0.0f;
        #pragma GCC ivdep
        for (size_t i = 0; i < n; ++i) {
            sum += xs[i] + ys[i] + zs[i];
        }
        benchmark::DoNotOptimize(sum);
    }

    setCounters(state);
}

BENCHMARK_REGISTER_F(Fixture, AtomStorageReadXYZ)
    ->Arg(5)
    ->Arg(10)
    ->Arg(25)
    ->Arg(47);

// @bench_meta {"id":"Fixture/AtomStorageWriteVelocity","label":"AtomStorage: write velocity","group":"Simulation/Data Layout"}
BENCHMARK_DEFINE_F(Fixture, AtomStorageWriteVelocity)(benchmark::State& state) {
    rebuildScene();
    warmupScene();

    AtomStorage& atoms = simulation_->atoms();
    const size_t n = atoms.mobileCount();

    for (auto _ : state) {
        float* RESTRICT vx = atoms.vx().data();
        float* RESTRICT vy = atoms.vy().data();
        float* RESTRICT vz = atoms.vz().data();

        #pragma GCC ivdep
        for (size_t i = 0; i < n; ++i) {
            vx[i] += 0.001f;
            vy[i] -= 0.001f;
            vz[i] += 0.002f;
        }

        benchmark::ClobberMemory();
    }

    setCounters(state);
}

BENCHMARK_REGISTER_F(Fixture, AtomStorageWriteVelocity)
    ->Arg(5)
    ->Arg(10)
    ->Arg(25)
    ->Arg(47);

// @bench_meta {"id":"Fixture/AtomStorageSwapPrevCurrentForces","label":"AtomStorage: swap force buffers","group":"Simulation/Data Layout"}
BENCHMARK_DEFINE_F(Fixture, AtomStorageSwapPrevCurrentForces)(benchmark::State& state) {
    rebuildScene();
    warmupScene();

    AtomStorage& atoms = simulation_->atoms();

    for (auto _ : state) {
        atoms.swapPrevCurrentForces();
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

BENCHMARK_REGISTER_F(Fixture, AtomStorageSwapPrevCurrentForces)
    ->Arg(5)
    ->Arg(10)
    ->Arg(25)
    ->Arg(47);
