#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Lattice/Kernel/Consts.hpp>

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

    void move(const glm::vec3& delta) noexcept {
        transform_.position += delta;
    }

    void rotate(const glm::quat& delta) noexcept {
        transform_.rotation = glm::normalize(delta * transform_.rotation);
    }

    void scale(const glm::vec3& factor) noexcept {
        transform_.scale *= factor;
    }

    void setPosition(const glm::vec3& position) noexcept {
        transform_.position = position;
    }

    void setRotation(const glm::quat& rotation) noexcept {
        transform_.rotation = glm::normalize(rotation);
    }

    void setScale(const glm::vec3& scale) noexcept {
        transform_.scale = scale;
    }

private:
    Transform transform_;
};
