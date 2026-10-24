#pragma once

#include <cstdint>
#include <span>

#include <glm/glm.hpp>

#include "Program.hpp"
#include "Shape.hpp"

namespace Boundaries {

/// Distance from a point to the nearest wall and what that wall is.
struct Sample {
    float distance;     // > 0 inside the region, < 0 in a wall, |distance| to the surface
    uint32_t material;  // property index of the nearest surface
    uint32_t motion;    // rigid motion index of the nearest surface
};

/**
 * Evaluates a geometry program on the CPU (the same program runs on the GPU).
 *
 * Points are evaluated in blocks: one instruction at a time over a whole block, so
 * the dispatch is per block, not per point, and the inner loops are uniform.
 */
class Geometry final {
public:
    explicit Geometry(Program program);
    explicit Geometry(const Shape& shape) : Geometry(shape.compile()) {}

    Sample sample(glm::vec3 point) const;
    float distance(glm::vec3 point) const { return sample(point).distance; }

    /// Unit gradient of the distance: away from the wall, into the region.
    /// Central differences of size `step` (tetrahedral, four evaluations).
    glm::vec3 normal(glm::vec3 point, float step = 1e-4f) const;

    /// Distances (and optionally materials) of SoA points.
    void evaluate(std::span<const float> x, std::span<const float> y, std::span<const float> z,
                  std::span<float> distance, std::span<uint32_t> material = {}) const;

    const Program& program() const noexcept { return program_; }

private:
    /// Distances and the index of the primitive owning the nearest surface.
    void run(std::span<const float> x, std::span<const float> y, std::span<const float> z,
             std::span<float> distance, std::span<uint32_t> primitive) const;

    Program program_;
};

}
