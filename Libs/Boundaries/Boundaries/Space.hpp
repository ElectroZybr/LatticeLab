#pragma once

#include <span>

#include <glm/glm.hpp>

namespace Boundaries {

/**
 * The coordinate space of a simulation: an axis-aligned box and its periodicity.
 *
 * The box is a frame, not the shape of the region: spatial hashing is built on it and
 * the shape lives in Geometry. Along a periodic axis the space repeats with period
 * size: positions wrap into [origin, origin + size) and separations take the minimum
 * image. Along other axes nothing changes (walls belong to Geometry).
 */
class Space final {
public:
    Space(glm::vec3 origin, glm::vec3 size, glm::bvec3 periodic = glm::bvec3(false));

    glm::vec3 origin() const noexcept { return origin_; }
    glm::vec3 size() const noexcept { return size_; }
    glm::bvec3 periodic() const noexcept { return periodic_; }

    /// Nearest periodic image of a separation `a − b` (unchanged along non-periodic axes).
    glm::vec3 minimumImage(glm::vec3 delta) const noexcept;

    /// Position wrapped into the box along periodic axes, from any number of periods away.
    glm::vec3 wrap(glm::vec3 position) const noexcept;

    /// Wrap SoA columns in place (one pass per periodic axis).
    void wrap(std::span<float> x, std::span<float> y, std::span<float> z) const;

    /// Inside [origin, origin + size) along every axis.
    bool contains(glm::vec3 position) const noexcept;

private:
    glm::vec3 origin_;
    glm::vec3 size_;
    glm::vec3 inverse_;
    glm::bvec3 periodic_;
};

}
