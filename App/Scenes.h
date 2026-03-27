#pragma once

#include "Engine/Simulation.h"
#include "Engine/physics/AtomData.h"

namespace Scenes {
    inline void crystal(Simulation& sim, int n, AtomData::Type type, bool is3d,
                        double padding = 3.0, double margin = 15.0) {
        const double side = n * padding + padding + 2.0 * margin;
        const double half = side / 2.0;

        sim.setSizeBox(
            Vec3f(-half, -half, is3d ? -half : sim.sim_box.start.z),
            Vec3f(half, half, is3d ? half : sim.sim_box.end.z)
        );

        const Vec3f vecMargin(margin, margin, is3d ? margin : 0.0);
        const int zMax = is3d ? n : 1;

        for (int x = 1; x <= n; ++x) {
            for (int y = 1; y <= n; ++y) {
                for (int z = 1; z <= zMax; ++z) {
                    sim.createAtom(Vec3f(x, y, z) * padding + vecMargin,
                                   Vec3f::Random() * 0.5, type);
                }
            }
        }
    }
} // namespace Scenes
