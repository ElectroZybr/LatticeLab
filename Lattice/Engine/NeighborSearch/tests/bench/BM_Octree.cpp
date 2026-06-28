#include <benchmark/benchmark.h>

#include <optional>

#include "Fixture.h"
#include "Lattice/Engine/NeighborSearch/BarnesHut/Octree.h"
#include "Lattice/Plugins/ClassicMD/ForceFields/CoulombForceField.h"

namespace {
    void assignAlternatingCharges(AtomStorage& atoms) {
        for (size_t i = 0; i < atoms.size(); ++i) {
            atoms.charge()[i] = (i & 1u) == 0u ? 1.0f : -1.0f;
        }
    }

    void clearForceAndEnergy(AtomStorage& atoms) {
        for (size_t i = 0; i < atoms.size(); ++i) {
            atoms.fx()[i] = 0.0f;
            atoms.fy()[i] = 0.0f;
            atoms.fz()[i] = 0.0f;
            atoms.energy()[i] = 0.0f;
        }
    }
}

// @bench_meta {"id":"Fixture/BarnesHutOctreeBuild","label":"Barnes-Hut Octree Build","group":"Simulation/Long-range Coulomb"}
BENCHMARK_DEFINE_F(Fixture, BarnesHutOctreeBuild)(benchmark::State& state) {
    rebuildScene();
    warmupScene();

    auto& atoms = simulation_->atoms();
    auto& grid = simulation_->world().getGrid();
    std::optional<OctreeNode> root;

    for (auto _ : state) {
        state.PauseTiming();
        root.reset();
        root.emplace();
        state.ResumeTiming();

        root->build(atoms, grid);
        benchmark::DoNotOptimize(*root);
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

BENCHMARK_REGISTER_F(Fixture, BarnesHutOctreeBuild)
    ->Arg(5)
    ->Arg(10)
    ->Arg(25)
    ->Arg(47);

// @bench_meta {"id":"Fixture/LongRangeCoulomb","label":"Long-range Coulomb","group":"Simulation/Long-range Coulomb"}
BENCHMARK_DEFINE_F(Fixture, LongRangeCoulomb)(benchmark::State& state) {
    rebuildScene();
    warmupScene();

    auto& atoms = simulation_->atoms();
    auto& grid = simulation_->world().getGrid();
    assignAlternatingCharges(atoms);

    OctreeNode root;
    root.build(atoms, grid);

    CoulombForceField coulombForceField;
    constexpr float kTheta = 0.7f;

    for (auto _ : state) {
        clearForceAndEnergy(atoms);

        for (size_t atomIndex = 0; atomIndex < atoms.size(); ++atomIndex) {
            float forceX = atoms.fx()[atomIndex];
            float forceY = atoms.fy()[atomIndex];
            float forceZ = atoms.fz()[atomIndex];
            float potentialEnergy = atoms.energy()[atomIndex];

            coulombForceField.computeForce(atoms, atomIndex, root, kTheta, forceX, forceY, forceZ, potentialEnergy);

            atoms.fx()[atomIndex] = forceX;
            atoms.fy()[atomIndex] = forceY;
            atoms.fz()[atomIndex] = forceZ;
            atoms.energy()[atomIndex] = potentialEnergy;
        }

        benchmark::DoNotOptimize(atoms.fx()[0]);
        benchmark::DoNotOptimize(atoms.energy()[0]);
        benchmark::ClobberMemory();
    }

    setCounters(state);
}

BENCHMARK_REGISTER_F(Fixture, LongRangeCoulomb)
    ->Arg(5)
    ->Arg(10)
    ->Arg(25)
    ->Arg(47);
