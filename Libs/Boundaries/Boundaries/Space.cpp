#include "Space.hpp"

#include <cmath>

#include <Lattice/Tools/Exception.hpp>

namespace Boundaries {

using Error = Lattice::Exception<Space>;

namespace {

float wrapCoordinate(float value, float origin, float size, float inverse) noexcept {
    const float local = value - origin;
    float wrapped = local - size * std::floor(local * inverse);
    // Rounding can land exactly on the upper bound; keep the half-open interval.
    if (wrapped >= size)
        wrapped -= size;
    return origin + wrapped;
}

}

Space::Space(glm::vec3 origin, glm::vec3 size, glm::bvec3 periodic)
    : origin_(origin), size_(size), inverse_(1.0f / size), periodic_(periodic) {
    for (int axis = 0; axis < 3; ++axis)
        if (!std::isfinite(size[axis]) || size[axis] <= 0.0f || !std::isfinite(origin[axis]))
            throw Error("space size must be positive and finite on every axis (axis {}: origin {}, size {})",
                axis, origin[axis], size[axis]);
}

glm::vec3 Space::minimumImage(glm::vec3 delta) const noexcept {
    for (int axis = 0; axis < 3; ++axis)
        if (periodic_[axis])
            delta[axis] -= size_[axis] * std::round(delta[axis] * inverse_[axis]);
    return delta;
}

glm::vec3 Space::wrap(glm::vec3 position) const noexcept {
    for (int axis = 0; axis < 3; ++axis)
        if (periodic_[axis])
            position[axis] = wrapCoordinate(position[axis], origin_[axis], size_[axis], inverse_[axis]);
    return position;
}

void Space::wrap(std::span<float> x, std::span<float> y, std::span<float> z) const {
    if (x.size() != y.size() || x.size() != z.size())
        throw Error("columns differ in length: {}, {}, {}", x.size(), y.size(), z.size());
    const std::span<float> columns[3] = {x, y, z};
    for (int axis = 0; axis < 3; ++axis) {
        if (!periodic_[axis])
            continue;
        const float origin = origin_[axis], size = size_[axis], inverse = inverse_[axis];
        for (float& value : columns[axis])
            value = wrapCoordinate(value, origin, size, inverse);
    }
}

bool Space::contains(glm::vec3 position) const noexcept {
    const glm::vec3 local = position - origin_;
    return glm::all(glm::greaterThanEqual(local, glm::vec3(0.0f))) && glm::all(glm::lessThan(local, size_));
}

}
