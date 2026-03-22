#include <cmath>

#include <benchmark/benchmark.h>

#include "Engine/Simulation.h"
#include "Engine/physics/integrators/VerletScheme.h"

void crystal(Simulation& simulation, int n, Atom::Type type, bool is3d, double padding = 3, double margin = 15) {
    const int sib_box_size = n * padding + padding + 2.0*margin;
    const double half = sib_box_size / 2.0;

    simulation.setSizeBox(
        Vec3D(-half, -half, is3d ? -half : simulation.sim_box.start.z),
        Vec3D( half,  half, is3d ?  half : simulation.sim_box.end.z)
    );

    const int zMax = is3d ? n : 1;
    const Vec3D vecMargin(margin, margin, is3d? margin : 0);
    for (int x = 1; x <= n; x++)
        for (int y = 1; y <= n; y++)
            for (int z = 1; z <= zMax; z++)
                simulation.createAtom( Vec3D(x, y, z) * padding + vecMargin, Vec3D::Random() * 0.5, type);
}

constexpr double dt = 0.01;

static void BM_Simulation(benchmark::State& state) {
    const int countAtoms = state.range(0);
    const int n = std::cbrt(countAtoms);

    SimBox simBox(Vec3D(-30, -30, -30), Vec3D(10, 10, 10));
    Simulation simulation(simBox);

    crystal(simulation, n, Atom::Type::H, true);

    for (auto _ : state) {
        simulation.update(dt);
    }
}

static void BM_Predict(benchmark::State& state) {
    const int countAtoms = state.range(0);
    const int n = std::cbrt(countAtoms);

    SimBox simBox(Vec3D(-30, -30, -30), Vec3D(30, 30, 30));
    Simulation simulation(simBox);
    crystal(simulation, n, Atom::Type::H, true);

    for (auto _ : state) {
        for (Atom& atom : simulation.atoms)
            VerletScheme::predict(atom, dt);
        benchmark::ClobberMemory();
    }
}

// static void BM_ComputeForces(benchmark::State& state) {
//    const int countAtoms = state.range(0);
//    const int n = std::cbrt(countAtoms);
//     SimBox simBox(Vec3D(-30, -30, -30), Vec3D(30, 30, 30));
//     Simulation simulation(simBox);
//     crystal(simulation, n, Atom::Type::H, true);

//     for (auto _ : state) {
//         StepOps::computeForces(simulation.atoms, simulation.sim_box, simulation.forceField, dt);
//         benchmark::ClobberMemory();
//     }
// }

static void BM_Correct(benchmark::State& state) {
    const int countAtoms = state.range(0);
    const int n = std::cbrt(countAtoms);
    SimBox simBox(Vec3D(-30, -30, -30), Vec3D(30, 30, 30));
    Simulation simulation(simBox);
    crystal(simulation, n, Atom::Type::H, true);

    for (auto _ : state) {
        for (Atom& atom : simulation.atoms) {
            if (!atom.isFixed) {
                VerletScheme::correct(atom, dt);
            }
        }
        benchmark::ClobberMemory();
    }
}


BENCHMARK(BM_Simulation)->Args({10*10*10})->Args({20*20*20})->Args({30*30*30});
BENCHMARK(BM_Predict)->Args({10*10*10})->Args({20*20*20})->Args({30*30*30});
// BENCHMARK(BM_ComputeForces)->Args({10*10*10})->Args({20*20*20})->Args({30*30*30});
BENCHMARK(BM_Correct)->Args({10*10*10})->Args({20*20*20})->Args({30*30*30});
