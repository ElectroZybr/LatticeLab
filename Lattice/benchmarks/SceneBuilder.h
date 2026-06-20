#pragma once

#include "Lattice/Engine/physics/IIntegrator.h"

namespace Lattice {
    class Simulation;
}

namespace Benchmarks {
    enum class SceneKind {
        Gas,
        Crystal,
    };

    class Scenes {
    public:
        static void build(Lattice::Simulation& simulation, SceneKind scene, int atomCount);
    private:
        static void buildGas(Lattice::Simulation& simulation, int atomCount);
        static void buildCrystal(Lattice::Simulation& simulation, int atomCount);
    };
}
