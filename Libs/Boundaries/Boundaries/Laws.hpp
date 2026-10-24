#pragma once

#include <cmath>
#include <cstdint>

#include <glm/glm.hpp>

namespace Boundaries {

/// What a surface does to particles: a law and its parameters. Fixed size plain data,
/// a GPU record as is (one table for the CPU and the GPU).
struct Material {
    uint32_t law = 0;
    float params[7] = {};
};
static_assert(sizeof(Material) == 32, "Material is a GPU record: keep it 32 bytes");

/// The stage of a step a law acts in.
enum class Stage : uint32_t {
    None = 0,
    Force = 1,      // adds a force while forces are computed
    Constrain = 2,  // corrects positions and velocities after positions moved
};

/// State of one particle next to a surface, as a law sees it.
struct Contact {
    glm::vec3 position;
    glm::vec3 velocity;
    float distance;     // > 0 in the region, < 0 in the wall
    glm::vec3 normal;   // into the region
};

/**
 * Laws: one struct each — id, stage, parameters, the CPU rule (and later its WGSL).
 * A new law is a new struct plus an entry in Walls' dispatch; nothing else changes.
 */
namespace Laws {

/// No interaction: particles pass through the surface.
struct Open {
    static constexpr uint32_t id = 0;
    static constexpr Stage stage = Stage::None;
    static Material make() { return {.law = id}; }
};

/// Impenetrable wall: a particle in the wall is moved back onto the surface and the
/// normal velocity reflected, scaled by restitution (1 — elastic).
struct Hard {
    static constexpr uint32_t id = 1;
    static constexpr Stage stage = Stage::Constrain;
    /// Projection passes for a particle in a wall (blends give distance bounds).
    static constexpr int ProjectionPasses = 3;
    static Material make(float restitution = 1.0f) { return {.law = id, .params = {restitution}}; }

    static glm::vec3 reflect(glm::vec3 velocity, glm::vec3 normal, const Material& material) {
        const float normalSpeed = glm::dot(velocity, normal);
        return normalSpeed < 0.0f ? velocity - (1.0f + material.params[0]) * normalSpeed * normal : velocity;
    }
};

/// Repulsive layer of thickness `range`: U(d) = k / n · (range − d)^n for d < range,
/// force F = k · (range − d)^(n − 1) along the normal. n = 2 — harmonic; the old soft
/// walls were n = 6.
struct Soft {
    static constexpr uint32_t id = 2;
    static constexpr Stage stage = Stage::Force;
    static Material make(float stiffness, float range, float exponent = 2.0f) {
        return {.law = id, .params = {stiffness, range, exponent}};
    }
    static float range(const Material& material) { return material.params[1]; }

    static float potential(float distance, const Material& material) {
        const float depth = material.params[1] - distance;
        return depth > 0.0f ? material.params[0] / material.params[2] * std::pow(depth, material.params[2]) : 0.0f;
    }
    static glm::vec3 force(const Contact& contact, const Material& material) {
        const float depth = material.params[1] - contact.distance;
        return depth > 0.0f ? material.params[0] * std::pow(depth, material.params[2] - 1.0f) * contact.normal
                            : glm::vec3(0.0f);
    }
};

}

}
