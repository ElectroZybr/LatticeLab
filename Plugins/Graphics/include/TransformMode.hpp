#pragma once

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Lattice/Kernel/Node.hpp"
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
    void configure(Lattice::Node& node) {
        camera_ = node.focus<Camera>("Camera");
        viewport_ = node.requireParent<Viewport>();

        node.bind("pan", &pan_, [this](glm::vec2 delta) {
            if (!camera_ || !viewport_) return;
            const glm::vec3 d = panDelta(*camera_, *viewport_, delta, pivot_);
            camera_->move(d);
            pivot_ += d;
        });

        node.bind("zoom", &zoom_, [this](float delta) {
            if (!camera_) return;
            auto& t = camera_->transform();
            glm::vec3 offset = t.position - pivot_;
            const float distance = glm::length(offset);
            if (distance <= 1e-6f) return;
            const float newDistance = std::max(minDistance_, distance * (1.0f - delta * zoomSpeed_));
            t.position = pivot_ + glm::normalize(offset) * newDistance;
        });
    }

private:
    Lattice::Focus<Camera> camera_;
    Lattice::Ref<Viewport> viewport_;
    glm::vec2 pan_{};
    float zoom_ = 0.0f;
    glm::vec3 pivot_{0.0f};
    float zoomSpeed_ = 0.1f;
    float minDistance_ = 0.01f;
};

class FreeCameraController final : public TransformController {
public:
    void configure(Lattice::Node& node) {
        camera_ = node.focus<Camera>("Camera");
        viewport_ = node.requireParent<Viewport>();

        node.bind("look", &look_, [this](glm::vec2 delta) {
            if (!camera_) return;
            auto& r = camera_->transform().rotation;
            const float yaw = -delta.x * lookSensitivity_, pitch = -delta.y * lookSensitivity_;
            r = glm::normalize(glm::angleAxis(yaw, glm::vec3{0, 1, 0}) * r);
            r = glm::normalize(glm::angleAxis(pitch, glm::normalize(cameraRight(*camera_))) * r);
        });

        node.bind("zoom", &zoom_, [this](float delta) {
            if (camera_) camera_->move(cameraForward(*camera_) * delta * zoomSpeed_);
        });

        node.bind("pan", &pan_, [this](glm::vec2 delta) {
            if (!camera_ || !viewport_) return;
            const glm::vec3 planePoint = camera_->transform().position + cameraForward(*camera_) * panDistance_;
            camera_->move(panDelta(*camera_, *viewport_, delta, planePoint));
        });

        node.bind("move", &move_, [this](glm::vec3 v) {
            if (!camera_) return;
            camera_->move(cameraRight(*camera_) * v.x * moveSpeed_ + cameraUp(*camera_) * v.y * moveSpeed_ + cameraForward(*camera_) * v.z * moveSpeed_);
        });
    }

private:
    Lattice::Focus<Camera> camera_;
    Lattice::Ref<Viewport> viewport_;
    glm::vec3 move_{};
    glm::vec2 look_{}, pan_{};
    float zoom_ = 0.0f;
    float lookSensitivity_ = 0.003f, moveSpeed_ = 0.1f, zoomSpeed_ = 0.2f, panDistance_ = 1.0f;
};

class OrbitCameraController final : public TransformController {
public:
    void configure(Lattice::Node& node) {
        camera_ = node.focus<Camera>("Camera");
        viewport_ = node.requireParent<Viewport>();

        node.bind("orbit", &orbit_, [this](glm::vec2 delta) { if (camera_ && viewport_) orbit(delta); });

        node.bind("pan", &pan_, [this](glm::vec2 delta) {
            if (!camera_ || !viewport_) return;
            const glm::vec3 d = panDelta(*camera_, *viewport_, delta, pivot_);
            camera_->move(d);
            pivot_ += d;
        });

        node.bind("zoom", &zoom_, [this](float delta) { if (camera_) zoom(delta); });
        lookAtPivot();
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
    Lattice::Focus<Camera> camera_;
    Lattice::Ref<Viewport> viewport_;
    glm::vec2 orbit_{}, pan_{};
    float zoom_ = 0.0f;
    glm::vec3 pivot_{0.0f};
    float orbitSpeed_ = 1.0f, zoomSpeed_ = 0.1f, minDistance_ = 0.01f;
};

class TrackballCameraController final : public TransformController {
public:
    void configure(Lattice::Node& node) {
        camera_ = node.focus<Camera>("Camera");
        viewport_ = node.requireParent<Viewport>();

        node.bind("orbit", &orbit_, [this](glm::vec2 delta) { if (camera_ && viewport_) orbit(delta); });

        node.bind("pan", &pan_, [this](glm::vec2 delta) {
            if (!camera_ || !viewport_) return;
            const glm::vec3 d = panDelta(*camera_, *viewport_, delta, pivot_);
            camera_->move(d);
            pivot_ += d;
        });

        node.bind("zoom", &zoom_, [this](float delta) { if (camera_) zoom(delta); });

        orbitUp_ = camera_->transform().rotation * glm::vec3{0, 1, 0};
        if (glm::dot(orbitUp_, orbitUp_) <= 1e-8f) orbitUp_ = {0, 1, 0};
        orbitUp_ = glm::normalize(orbitUp_);

        lookAtPivot();
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
    Lattice::Focus<Camera> camera_;
    Lattice::Ref<Viewport> viewport_;
    glm::vec2 orbit_{}, pan_{};
    float zoom_ = 0.0f;
    glm::vec3 pivot_{0.0f}, orbitUp_{0, 1, 0};
    float orbitSpeed_ = 1.0f, zoomSpeed_ = 0.1f, minDistance_ = 0.01f;
};