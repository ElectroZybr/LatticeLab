#pragma once

#include <span>
#include <vector>

#include "Geometry.hpp"
#include "Laws.hpp"

namespace Boundaries {

/// Columns of the particles a boundary acts on: pass the mobile ones only (fixed
/// particles are not moved by walls). Velocities are needed by Constrain, forces by
/// Force; a stage leaves the other columns untouched.
struct ParticleColumns {
    std::span<float> x, y, z;
    std::span<float> vx, vy, vz;
    std::span<float> fx, fy, fz;
};

/**
 * The response of the geometry's surfaces: each surface has a material (its law and
 * parameters), the laws act in their stage of the step.
 *
 * Particles are processed in chunks: distances for the whole chunk in one pass, then
 * only particles close enough to a surface get normals and a law. A particle far
 * inside the region costs one distance evaluation per stage.
 */
class Walls final {
public:
    /// Material 0 is the default; every material index of the program must exist.
    Walls(const Geometry& geometry, std::vector<Material> materials = {Laws::Hard::make()});

    /// Force stage: add wall forces (Soft) to fx, fy, fz.
    void addForces(const ParticleColumns& particles) const;
    /// Constrain stage: move particles out of walls and reflect velocities (Hard).
    void constrain(const ParticleColumns& particles) const;

    const std::vector<Material>& materials() const noexcept { return materials_; }
    const Geometry& geometry() const noexcept { return geometry_; }

    /// Projection passes for a particle in a wall (blends give distance bounds).
    static constexpr int ProjectionPasses = 3;

private:
    template<class Apply>
    void forNear(const ParticleColumns& particles, float within, Apply&& apply) const;

    const Geometry& geometry_;
    std::vector<Material> materials_;
    float forceRange_ = 0.0f;   // widest Soft layer: farther particles feel no force
};

}
