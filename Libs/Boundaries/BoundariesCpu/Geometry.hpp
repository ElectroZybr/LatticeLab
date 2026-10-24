#pragma once

#include <cstdint>
#include <span>

#include <glm/glm.hpp>

#include <Boundaries/Program.hpp>
#include <Boundaries/Shape.hpp>

namespace Boundaries::Cpu {

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

    /// Unit gradient of the distance: away from the wall, into the region. Analytic —
    /// carried through the program with the distance (one evaluation).
    glm::vec3 normal(glm::vec3 point) const;

    /// Normals of SoA points (analytic, one pass).
    void normals(std::span<const float> x, std::span<const float> y, std::span<const float> z,
                 std::span<float> nx, std::span<float> ny, std::span<float> nz) const;

    /// Distances (and optionally materials) of SoA points.
    void evaluate(std::span<const float> x, std::span<const float> y, std::span<const float> z,
                  std::span<float> distance, std::span<uint32_t> material = {}) const;

    const Program& program() const noexcept { return program_; }

private:
    /// Distances, the index of the primitive owning the nearest surface and, with
    /// Gradient, unit normals.
    template<bool Gradient>
    void run(std::span<const float> x, std::span<const float> y, std::span<const float> z,
             std::span<float> distance, std::span<uint32_t> primitive,
             std::span<float> nx, std::span<float> ny, std::span<float> nz) const;

    Program program_;
};

}
