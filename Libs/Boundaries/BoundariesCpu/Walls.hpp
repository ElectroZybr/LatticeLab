#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include <Boundaries/Laws.hpp>
#include <Boundaries/ParticleLayout.hpp>
#include <StdData/SoA.hpp>

#include "Geometry.hpp"

namespace Boundaries::Cpu {

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
    Walls(const Geometry& geometry, ParticleLayout layout, std::vector<Material> materials = {Laws::Hard::make()});

    /// Force stage: add wall forces (Soft) to the force columns of rows [0, rows).
    void addForces(StdData::SoA& particles, size_t rows) const;
    /// Constrain stage: move rows [0, rows) out of walls and reflect their velocities (Hard).
    /// Pass the mobile rows: fixed particles are not moved by walls.
    void constrain(StdData::SoA& particles, size_t rows) const;

    const ParticleLayout& layout() const noexcept { return layout_; }

    const std::vector<Material>& materials() const noexcept { return materials_; }
    const Geometry& geometry() const noexcept { return geometry_; }

private:
    struct Columns {
        std::span<float> x, y, z, vx, vy, vz, fx, fy, fz;
    };
    Columns columns(StdData::SoA& particles, size_t rows, bool velocities, bool forces) const;

    const Geometry& geometry_;
    ParticleLayout layout_;
    std::vector<Material> materials_;
    float forceRange_ = 0.0f;   // widest Soft layer: farther particles feel no force
};

}
