#pragma once

#include <string>

namespace Boundaries {

/// Which columns of a particle table the boundaries read and write, by name: the same
/// layout serves every backend (a StdData::SoA on the CPU, a GpuSoA on the GPU).
/// Columns are f32. A stage uses only what it needs: Constrain — positions and
/// velocities, Force — positions and forces.
struct ParticleLayout {
    std::string x, y, z;
    std::string vx, vy, vz;
    std::string fx, fy, fz;
};

}
