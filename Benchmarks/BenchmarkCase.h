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
        Vec3f boxStart = Vec3f(-50.0, -50.0, 0.0);
        Vec3f boxEnd = Vec3f(50.0, 50.0, 6.0);
        int cellSize = 5;
    };
}
