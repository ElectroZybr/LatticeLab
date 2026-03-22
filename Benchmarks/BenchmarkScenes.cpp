#include "BenchmarkScenes.h"

#include <algorithm>
#include <cmath>

#include "Engine/Simulation.h"

namespace Benchmarks {
namespace {

int gridSideFromCount(int atomCount) {
    return std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<double>(atomCount)))));
}

int cubeSideFromCount(int atomCount) {
    return std::max(1, static_cast<int>(std::ceil(std::cbrt(static_cast<double>(atomCount)))));
}

}  // namespace

void BenchmarkScenes::build(Simulation& simulation, const BenchmarkCase& benchmarkCase) {
    simulation.clear();
    simulation.setIntegrator(benchmarkCase.integrator);
    simulation.setSizeBox(benchmarkCase.boxStart, benchmarkCase.boxEnd, benchmarkCase.cellSize);

    switch (benchmarkCase.scene) {
    case SceneKind::Crystal2D:
        buildCrystal2D(simulation, benchmarkCase);
        break;
    case SceneKind::Crystal3D:
        buildCrystal3D(simulation, benchmarkCase);
        break;
    case SceneKind::RandomGas2D:
        buildRandomGas2D(simulation, benchmarkCase);
        break;
    }
}

void BenchmarkScenes::buildCrystal2D(Simulation& simulation, const BenchmarkCase& benchmarkCase) {
    constexpr double spacing = 3.0;
    const int side = gridSideFromCount(benchmarkCase.atomCount);

    int created = 0;
    for (int x = 0; x < side && created < benchmarkCase.atomCount; ++x) {
        for (int y = 0; y < side && created < benchmarkCase.atomCount; ++y) {
            const Vec3D pos(
                benchmarkCase.boxStart.x + 3.0 + x * spacing,
                benchmarkCase.boxStart.y + 3.0 + y * spacing,
                1.0);
            simulation.createAtom(pos, Vec3D::Random() * 0.5, Atom::Type::H);
            ++created;
        }
    }
}

void BenchmarkScenes::buildCrystal3D(Simulation& simulation, const BenchmarkCase& benchmarkCase) {
    constexpr double spacing = 3.0;
    const int side = cubeSideFromCount(benchmarkCase.atomCount);

    int created = 0;
    for (int x = 0; x < side && created < benchmarkCase.atomCount; ++x) {
        for (int y = 0; y < side && created < benchmarkCase.atomCount; ++y) {
            for (int z = 0; z < side && created < benchmarkCase.atomCount; ++z) {
                const Vec3D pos(
                    benchmarkCase.boxStart.x + 3.0 + x * spacing,
                    benchmarkCase.boxStart.y + 3.0 + y * spacing,
                    benchmarkCase.boxStart.z + 3.0 + z * spacing);
                simulation.createAtom(pos, Vec3D::Random() * 0.5, Atom::Type::H);
                ++created;
            }
        }
    }
}

void BenchmarkScenes::buildRandomGas2D(Simulation& simulation, const BenchmarkCase& benchmarkCase) {
    simulation.createRandomAtoms(Atom::Type::H, benchmarkCase.atomCount);
}

}
