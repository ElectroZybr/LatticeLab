#pragma once

#include "Engine/math/Vec3D.h"
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
    Vec3D boxStart = Vec3D(-50.0, -50.0, 0.0);
    Vec3D boxEnd = Vec3D(50.0, 50.0, 6.0);
    int cellSize = 5;
};

}  // namespace Benchmarks
