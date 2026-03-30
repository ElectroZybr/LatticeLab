#pragma once

#include "Engine/math/Vec3f.h"
#include "Engine/physics/Integrator.h"

namespace Benchmarks {
    enum class SceneKind {
        Crystal2D,
        Crystal3D,
        RandomGas2D,
    };

    struct BenchmarkCase {
        SceneKind scene = SceneKind::Crystal2D;
        Integrator::Scheme integrator = Integrator::Scheme::Verlet;

        int atomCount = 1000;
        Vec3f boxSize = Vec3f(100.0, 100.0, 6.0);
        int cellSize = 5;
    };
}
