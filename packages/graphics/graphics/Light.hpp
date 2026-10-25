#pragma once

#include <glm/vec3.hpp>

/// A light of a scene (a child of its viewport): directional — parallel rays along
/// `direction`, or point — from `position`, fading as 1 / (1 + (d / range)²).
struct Light {
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    bool point = false;
    glm::vec3 direction{-0.4f, -1.0f, -0.6f};
    glm::vec3 position{0.0f};
    float range = 10.0f;
};
