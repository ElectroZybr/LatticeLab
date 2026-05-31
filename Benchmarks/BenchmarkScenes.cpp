#include "BenchmarkScenes.h"

#include <algorithm>
#include <cmath>

#include "App/Scenes.h"
#include "Lattice/Simulation.h"

namespace Benchmarks {
    int squareSideFromCount(int atomCount) { return std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<double>(atomCount))))); }

    int cubeSideFromCount(int atomCount) { return std::max(1, static_cast<int>(std::ceil(std::cbrt(static_cast<double>(atomCount))))); }

    void BenchmarkScenes::build(Lattice::Simulation& simulation, const BenchmarkCase& benchmarkCase) {
        simulation.clear();
        simulation.world().getIntegrator().setScheme(benchmarkCase.integrator);

        switch (benchmarkCase.scene) {
        case SceneKind::IdealCrystal3D:
            buildCrystal2D(simulation, benchmarkCase);
            break;            
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

    void BenchmarkScenes::buildIdealCrystal3D(Lattice::Simulation& simulation, const BenchmarkCase& benchmarkCase) {
        const int side = cubeSideFromCount(benchmarkCase.atomCount);
        Scenes::hexLattice(simulation, side, AtomData::Type::Z, true);
    }

    void BenchmarkScenes::buildCrystal2D(Lattice::Simulation& simulation, const BenchmarkCase& benchmarkCase) {
        const int side = squareSideFromCount(benchmarkCase.atomCount);
        Scenes::crystal(simulation, side, AtomData::Type::Z, false);
    }

    void BenchmarkScenes::buildCrystal3D(Lattice::Simulation& simulation, const BenchmarkCase& benchmarkCase) {
        const int side = cubeSideFromCount(benchmarkCase.atomCount);
        Scenes::crystal(simulation, side, AtomData::Type::Z, true);
    }

    void BenchmarkScenes::buildRandomGas2D(Lattice::Simulation& simulation, const BenchmarkCase& benchmarkCase) {
        simulation.setSizeBox(benchmarkCase.boxSize, benchmarkCase.cellSize);
        Scenes::randomGasInCurrentBox(simulation, benchmarkCase.atomCount, AtomData::Type::H, false);
    }
}
