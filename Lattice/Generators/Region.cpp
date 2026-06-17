#include "Lattice/Generators/Region.hpp"

#include <cmath>
#include <algorithm>

#include <glm/geometric.hpp>

namespace Lattice::Generators {
namespace {

bool pointInEquilateralTriangle(float x, float y, float circumradius) {
    if (circumradius <= 0.0f) {
        return false;
    }

    const float sqrt3 = std::sqrt(3.0f);
    const glm::vec2 a(0.0f, circumradius);
    const glm::vec2 b(-0.5f * sqrt3 * circumradius, -0.5f * circumradius);
    const glm::vec2 c(0.5f * sqrt3 * circumradius, -0.5f * circumradius);
    const glm::vec2 p(x, y);

    const auto sign = [](glm::vec2 p1, glm::vec2 p2, glm::vec2 p3) {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    };

    const float d1 = sign(p, a, b);
    const float d2 = sign(p, b, c);
    const float d3 = sign(p, c, a);
    const bool hasNeg = (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
    const bool hasPos = (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);
    return !(hasNeg && hasPos);
}

} // namespace

bool Rectangle::contains(glm::vec3 point) const {
    const glm::vec3 halfBoxSize = 0.5f * boxSize;
    const glm::vec3 delta = point - center;

    return std::abs(delta.x) <= halfBoxSize.x &&
           std::abs(delta.y) <= halfBoxSize.y &&
           std::abs(delta.z) <= halfBoxSize.z;
}

Bounds Rectangle::bounds() const {
    const glm::vec3 halfBoxSize = 0.5f * boxSize;
    return {
        .min = center - halfBoxSize,
        .max = center + halfBoxSize,
    };
}

bool Sphere::contains(glm::vec3 point) const {
    const glm::vec3 delta = point - center;
    return glm::dot(delta, delta) <= sphereRadius * sphereRadius;
}

Bounds Sphere::bounds() const {
    const glm::vec3 extent(sphereRadius);
    return {
        .min = center - extent,
        .max = center + extent,
    };
}

bool Cylinder::contains(glm::vec3 point) const {
    const glm::vec3 delta = point - center;
    const float halfCylinderHeight = 0.5f * cylinderHeight;
    return delta.x * delta.x + delta.y * delta.y <= baseRadius * baseRadius &&
           std::abs(delta.z) <= halfCylinderHeight;
}

Bounds Cylinder::bounds() const {
    const glm::vec3 extent(baseRadius, baseRadius, 0.5f * cylinderHeight);
    return {
        .min = center - extent,
        .max = center + extent,
    };
}

bool Capsule::contains(glm::vec3 point) const {
    const glm::vec3 delta = point - center;
    const float halfCapsuleHeight = 0.5f * capsuleHeight;
    const float clampedZ = std::clamp(delta.z, -halfCapsuleHeight, halfCapsuleHeight);
    const glm::vec3 closest(0.0f, 0.0f, clampedZ);
    const glm::vec3 local = delta - closest;
    return glm::dot(local, local) <= capsuleRadius * capsuleRadius;
}

Bounds Capsule::bounds() const {
    const glm::vec3 extent(capsuleRadius, capsuleRadius, 0.5f * capsuleHeight + capsuleRadius);
    return {
        .min = center - extent,
        .max = center + extent,
    };
}

bool Torus::contains(glm::vec3 point) const {
    const glm::vec3 delta = point - center;
    const float radial = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    const float ring = radial - majorRadius;
    return ring * ring + delta.z * delta.z <= tubeRadius * tubeRadius;
}

Bounds Torus::bounds() const {
    const glm::vec3 extent(majorRadius + tubeRadius, majorRadius + tubeRadius, tubeRadius);
    return {
        .min = center - extent,
        .max = center + extent,
    };
}

bool TrianglePyramid::contains(glm::vec3 point) const {
    const glm::vec3 delta = point - center;
    const float halfPyramidHeight = 0.5f * pyramidHeight;
    if (delta.z < -halfPyramidHeight || delta.z > halfPyramidHeight) {
        return false;
    }

    const float heightFactor = (halfPyramidHeight - delta.z) / std::max(pyramidHeight, 1e-6f);
    const float localCircumradius = baseCircumradius * heightFactor;
    return pointInEquilateralTriangle(delta.x, delta.y, localCircumradius);
}

Bounds TrianglePyramid::bounds() const {
    const float halfBaseWidth = 0.5f * std::sqrt(3.0f) * baseCircumradius;
    const glm::vec3 extent(halfBaseWidth, baseCircumradius, 0.5f * pyramidHeight);
    return {
        .min = center - extent,
        .max = center + extent,
    };
}

bool TriangleBiPyramid::contains(glm::vec3 point) const {
    const glm::vec3 delta = point - center;
    const float halfBipyramidHeight = 0.5f * bipyramidHeight;
    if (std::abs(delta.z) > halfBipyramidHeight) {
        return false;
    }

    const float heightFactor = 1.0f - std::abs(delta.z) / std::max(halfBipyramidHeight, 1e-6f);
    const float localCircumradius = baseCircumradius * heightFactor;
    return pointInEquilateralTriangle(delta.x, delta.y, localCircumradius);
}

Bounds TriangleBiPyramid::bounds() const {
    const float halfBaseWidth = 0.5f * std::sqrt(3.0f) * baseCircumradius;
    const glm::vec3 extent(halfBaseWidth, baseCircumradius, 0.5f * bipyramidHeight);
    return {
        .min = center - extent,
        .max = center + extent,
    };
}

} // namespace Lattice::Generators
