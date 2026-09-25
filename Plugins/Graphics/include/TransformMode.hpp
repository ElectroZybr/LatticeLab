#pragma once

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Lattice/Kernel/NodeViews.hpp>

#include "Camera.hpp"
#include "TransformController.hpp"
#include "Viewport.hpp"

namespace {
inline glm::vec3 cameraForward(const Camera& c) { return c.transform().rotation * glm::vec3{0, 0, -1}; }
inline glm::vec3 cameraRight(const Camera& c) { return c.transform().rotation * glm::vec3{1, 0, 0}; }
inline glm::vec3 cameraUp(const Camera& c) { return c.transform().rotation * glm::vec3{0, 1, 0}; }

inline glm::vec3 panDelta(Camera& camera, Viewport& viewport, glm::vec2 delta, glm::vec3 planePoint) {
    return camera.screenDeltaToPlane(viewport.cursor(), delta, glm::vec2(viewport.size()), planePoint, cameraForward(camera));
}

inline glm::vec3 rotateAroundAxis(glm::vec3 v, glm::vec3 axis, float angle) {
    axis = glm::normalize(axis);
    const float c = std::cos(angle), s = std::sin(angle);
    return v * c + glm::cross(axis, v) * s + axis * glm::dot(axis, v) * (1.0f - c);
}
}

class Camera2DController final : public TransformController {
public:
    explicit Camera2DController(NodeBuild node) {
        node.param("pan", pan_);
        node.param("zoom", zoom_);
    }

    void configure(NodeConfigure node) {
        auto camera = node.focus<Camera>("Camera");
        camera_ = Ref<Camera>{camera.get()};
        viewport_ = node.require<Viewport>("Main");
    }

    void update() override {
        if (!camera_ || !viewport_) return;
        if (pan_ != glm::vec2{}) {
            const glm::vec3 d = panDelta(*camera_, *viewport_, pan_, pivot_);
            camera_->move(d);
            pivot_ += d;
            pan_ = {};
        }
        if (zoom_ != 0.0f) {
            auto& t = camera_->transform();
            glm::vec3 offset = t.position - pivot_;
            const float distance = glm::length(offset);
            if (distance > 1e-6f)
                t.position = pivot_ + glm::normalize(offset) * std::max(minDistance_, distance * (1.0f - zoom_ * zoomSpeed_));
            zoom_ = 0.0f;
        }
    }

private:
    Lattice::Ref<Camera> camera_;
    Lattice::Ref<Viewport> viewport_;
    glm::vec2 pan_{};
    float zoom_ = 0.0f;
    glm::vec3 pivot_{0.0f};
    float zoomSpeed_ = 0.1f;
    float minDistance_ = 0.01f;
};

class FreeCameraController final : public TransformController {
public:
    explicit FreeCameraController(NodeBuild node) {
        node.param("look", look_);
        node.param("zoom", zoom_);
        node.param("pan", pan_);
        node.param("move", move_);
    }

    void configure(NodeConfigure node) {
        auto camera = node.focus<Camera>("Camera");
        camera_ = Ref<Camera>{camera.get()};
        viewport_ = node.require<Viewport>("Main");
    }

    void update() override {
        if (!camera_) return;
        if (look_ != glm::vec2{}) {
            auto& r = camera_->transform().rotation;
            const float yaw = -look_.x * lookSensitivity_, pitch = -look_.y * lookSensitivity_;
            r = glm::normalize(glm::angleAxis(yaw, glm::vec3{0, 1, 0}) * r);
            r = glm::normalize(glm::angleAxis(pitch, glm::normalize(cameraRight(*camera_))) * r);
            look_ = {};
        }
        if (zoom_ != 0.0f) {
            camera_->move(cameraForward(*camera_) * zoom_ * zoomSpeed_);
            zoom_ = 0.0f;
        }
        if (viewport_ && pan_ != glm::vec2{}) {
            const glm::vec3 planePoint = camera_->transform().position + cameraForward(*camera_) * panDistance_;
            camera_->move(panDelta(*camera_, *viewport_, pan_, planePoint));
            pan_ = {};
        }
        if (move_ != glm::vec3{}) {
            camera_->move(cameraRight(*camera_) * move_.x * moveSpeed_ + cameraUp(*camera_) * move_.y * moveSpeed_ + cameraForward(*camera_) * move_.z * moveSpeed_);
            move_ = {};
        }
    }

private:
    Lattice::Ref<Camera> camera_;
    Lattice::Ref<Viewport> viewport_;
    glm::vec3 move_{};
    glm::vec2 look_{}, pan_{};
    float zoom_ = 0.0f;
    float lookSensitivity_ = 0.003f, moveSpeed_ = 0.1f, zoomSpeed_ = 0.2f, panDistance_ = 1.0f;
};

class OrbitCameraController final : public TransformController {
public:
    explicit OrbitCameraController(NodeBuild node) {
        node.param("orbit", orbit_);
        node.param("pan", pan_);
        node.param("zoom", zoom_);
    }

    void configure(NodeConfigure node) {
        auto camera = node.focus<Camera>("Camera");
        camera_ = Ref<Camera>{camera.get()};
        viewport_ = node.require<Viewport>("Main");
        lookAtPivot();
    }

    void update() override {
        if (!camera_) return;
        if (viewport_ && orbit_ != glm::vec2{}) { orbit(orbit_); orbit_ = {}; }
        if (viewport_ && pan_ != glm::vec2{}) {
            const glm::vec3 d = panDelta(*camera_, *viewport_, pan_, pivot_);
            camera_->move(d);
            pivot_ += d;
            pan_ = {};
        }
        if (zoom_ != 0.0f) { zoom(zoom_); zoom_ = 0.0f; }
    }

private:
    void orbit(glm::vec2 delta) {
        auto& t = camera_->transform();
        glm::vec3 offset = t.position - pivot_;
        const float distance = glm::length(offset);
        const glm::vec2 size = glm::vec2(viewport_->size());
        if (distance <= 1e-6f || size.x <= 0 || size.y <= 0) return;

        const float yaw = -delta.x / size.x * glm::two_pi<float>() * orbitSpeed_;
        const float pitch = -delta.y / size.y * glm::pi<float>() * orbitSpeed_;
        constexpr glm::vec3 worldUp{0, 1, 0};

        offset = glm::angleAxis(yaw, worldUp) * offset;
        glm::vec3 right = glm::cross(glm::normalize(-offset), worldUp);
        if (glm::dot(right, right) > 1e-8f) offset = glm::angleAxis(pitch, glm::normalize(right)) * offset;

        t.position = pivot_ + offset;
        lookAtPivot();
    }

    void zoom(float delta) {
        auto& t = camera_->transform();
        glm::vec3 offset = t.position - pivot_;
        const float distance = glm::length(offset);
        if (distance <= minDistance_) return;
        t.position = pivot_ + glm::normalize(offset) * std::max(minDistance_, distance * (1.0f - delta * zoomSpeed_));
        lookAtPivot();
    }

    void lookAtPivot() {
        auto& t = camera_->transform();
        const glm::vec3 dir = pivot_ - t.position;
        if (glm::dot(dir, dir) <= 1e-8f) return;
        t.rotation = glm::quatLookAt(glm::normalize(dir), glm::vec3{0, 1, 0});
    }

private:
    Lattice::Ref<Camera> camera_;
    Lattice::Ref<Viewport> viewport_;
    glm::vec2 orbit_{}, pan_{};
    float zoom_ = 0.0f;
    glm::vec3 pivot_{0.0f};
    float orbitSpeed_ = 1.0f, zoomSpeed_ = 0.1f, minDistance_ = 0.01f;
};

class TrackballCameraController final : public TransformController {
public:
    explicit TrackballCameraController(NodeBuild node) {
        node.param("orbit", orbit_);
        node.param("pan", pan_);
        node.param("zoom", zoom_);
    }

    void configure(NodeConfigure node) {
        auto camera = node.focus<Camera>("Camera");
        camera_ = Ref<Camera>{camera.get()};
        viewport_ = node.require<Viewport>("Main");

        orbitUp_ = camera_->transform().rotation * glm::vec3{0, 1, 0};
        if (glm::dot(orbitUp_, orbitUp_) <= 1e-8f) orbitUp_ = {0, 1, 0};
        orbitUp_ = glm::normalize(orbitUp_);

        lookAtPivot();
    }

    void update() override {
        if (!camera_) return;
        if (viewport_ && orbit_ != glm::vec2{}) { orbit(orbit_); orbit_ = {}; }
        if (viewport_ && pan_ != glm::vec2{}) {
            const glm::vec3 d = panDelta(*camera_, *viewport_, pan_, pivot_);
            camera_->move(d);
            pivot_ += d;
            pan_ = {};
        }
        if (zoom_ != 0.0f) { zoom(zoom_); zoom_ = 0.0f; }
    }

private:
    void orbit(glm::vec2 delta) {
        auto& t = camera_->transform();
        glm::vec3 offset = t.position - pivot_;
        const float distance = glm::length(offset);
        const glm::vec2 size = glm::vec2(viewport_->size());
        if (distance <= 1e-6f || size.x <= 0 || size.y <= 0) return;

        const float yaw = -delta.x / size.x * glm::two_pi<float>() * orbitSpeed_;
        const float pitch = -delta.y / size.y * glm::pi<float>() * orbitSpeed_;

        offset = rotateAroundAxis(offset, orbitUp_, yaw);

        glm::vec3 right = glm::cross(glm::normalize(-offset), orbitUp_);
        if (glm::dot(right, right) <= 1e-8f) return;
        right = glm::normalize(right);

        offset = rotateAroundAxis(offset, right, pitch);
        orbitUp_ = rotateAroundAxis(orbitUp_, right, pitch);
        offset = glm::normalize(offset) * distance;

        const glm::vec3 radial = glm::normalize(offset);
        orbitUp_ -= radial * glm::dot(orbitUp_, radial);
        if (glm::dot(orbitUp_, orbitUp_) > 1e-8f) orbitUp_ = glm::normalize(orbitUp_);

        t.position = pivot_ + offset;
        lookAtPivot();
    }

    void zoom(float delta) {
        auto& t = camera_->transform();
        glm::vec3 offset = t.position - pivot_;
        const float distance = glm::length(offset);
        if (distance <= minDistance_) return;
        t.position = pivot_ + glm::normalize(offset) * std::max(minDistance_, distance * (1.0f - delta * zoomSpeed_));
        lookAtPivot();
    }

    void lookAtPivot() {
        auto& t = camera_->transform();
        const glm::vec3 dir = pivot_ - t.position;
        if (glm::dot(dir, dir) <= 1e-8f) return;
        t.rotation = glm::quatLookAt(glm::normalize(dir), orbitUp_);
    }

private:
    Lattice::Ref<Camera> camera_;
    Lattice::Ref<Viewport> viewport_;
    glm::vec2 orbit_{}, pan_{};
    float zoom_ = 0.0f;
    glm::vec3 pivot_{0.0f}, orbitUp_{0, 1, 0};
    float orbitSpeed_ = 1.0f, zoomSpeed_ = 0.1f, minDistance_ = 0.01f;
};
