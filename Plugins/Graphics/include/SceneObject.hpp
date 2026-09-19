#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Lattice/Kernel/Component.hpp"

struct Transform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

class SceneObject : public Lattice::Component {
public:
    virtual ~SceneObject() = default;

    Transform& transform() noexcept { return transform_; }
    const Transform& transform() const noexcept { return transform_; }

private:
    Transform transform_;
};