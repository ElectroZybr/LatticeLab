#pragma once

#include "SceneObject.hpp"
#include <glm/glm.hpp>

#include "Lattice/Kernel/Node.hpp"

class Camera final : public SceneObject {
public:
    explicit Camera(Lattice::Node& node) {
        
    }

    glm::mat4 viewMatrix() const {
        const auto& t = transform();
        const glm::mat4 world =
            glm::translate(glm::mat4(1.0f), t.position) *
            glm::mat4_cast(t.rotation);

        return glm::inverse(world);
    }

    glm::mat4 projectionMatrix(float aspect) const {
        return glm::perspective(glm::radians(fov_), aspect, nearPlane_, farPlane_);
    }

    glm::mat4 viewProjection(float aspect) const {
        return projectionMatrix(aspect) * viewMatrix();
    }

    void setPerspective(float fov, float nearPlane, float farPlane);

    float fov() const noexcept { return fov_; }
    float nearPlane() const noexcept { return nearPlane_; }
    float farPlane() const noexcept { return farPlane_; }

private:
    float fov_ = 60.0f;
    float nearPlane_ = 0.1f;
    float farPlane_ = 1000.0f;
};