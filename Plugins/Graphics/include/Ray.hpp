#pragma once

#include <cmath>

#include <glm/glm.hpp>
#include <glm/geometric.hpp>

struct Ray {
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};

    [[nodiscard]] glm::vec3 at(float t) const noexcept {
        return origin + direction * t;
    }
};

struct RaySphereHit {
    float t = 0.0f;
    glm::vec3 point{0.0f};
};

inline bool raySphereIntersect(const Ray& ray, const glm::vec3& center, float radius, RaySphereHit& hit) {
    const glm::vec3 oc = ray.origin - center;

    const float a = glm::dot(ray.direction, ray.direction);
    const float b = 2.0f * glm::dot(oc, ray.direction);
    const float c = glm::dot(oc, oc) - radius * radius;

    const float d = b * b - 4.0f * a * c;
    if (d < 0.0f)
        return false;

    const float sqrtD = std::sqrt(d);

    float t = (-b - sqrtD) / (2.0f * a);
    if (t < 0.0f)
        t = (-b + sqrtD) / (2.0f * a);

    if (t < 0.0f)
        return false;

    hit.t = t;
    hit.point = ray.at(t);
    return true;
}

inline bool rayPlaneZIntersect(const Ray& ray, float z, glm::vec3& point) {
    if (std::abs(ray.direction.z) < 1e-6f)
        return false;

    const float t = (z - ray.origin.z) / ray.direction.z;
    if (t < 0.0f)
        return false;

    point = ray.at(t);
    return true;
}