#pragma once

#include "Ray.hpp"
#include "SceneObject.hpp"
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>

class Camera final : public SceneObject {
public:
    glm::mat4 viewMatrix() const {
        const auto& t = transform();
        const glm::mat4 world =
            glm::translate(glm::mat4(1.0f), t.position) *
            glm::mat4_cast(t.rotation);

        return glm::inverse(world);
    }

    glm::mat4 projectionMatrix(float aspect) const {
        return glm::perspectiveRH_ZO(glm::radians(fov_), aspect, nearPlane_, farPlane_);
    }

    glm::mat4 viewProjection(float aspect) const {
        return projectionMatrix(aspect) * viewMatrix();
    }

    glm::vec3 screenToPlane(
        glm::vec2 pixel,
        glm::vec2 viewportSize,
        glm::vec3 planePoint = {0.0f, 0.0f, 0.0f},
        glm::vec3 planeNormal = {0.0f, 0.0f, 1.0f}
    ) const {
        const Ray ray = screenRay(pixel, glm::uvec2(viewportSize));

        const float denom = glm::dot(planeNormal, ray.direction);
        if (std::abs(denom) <= 1e-6f)
            return planePoint;

        const float t = glm::dot(planePoint - ray.origin, planeNormal) / denom;
        return ray.at(t);
    }

    glm::vec3 screenDeltaToPlane(
        glm::vec2 pixel,
        glm::vec2 delta,
        glm::vec2 viewportSize,
        glm::vec3 planePoint = {0.0f, 0.0f, 0.0f},
        glm::vec3 planeNormal = {0.0f, 0.0f, 1.0f}
    ) const {
        return screenToPlane(pixel, viewportSize, planePoint, planeNormal) -
        screenToPlane(pixel + delta, viewportSize, planePoint, planeNormal);
    }

    Ray screenRay(glm::vec2 pixel, glm::uvec2 viewportSize) const {
        if (viewportSize.x == 0 || viewportSize.y == 0)
            return {};

        const float x = (2.0f * pixel.x) / float(viewportSize.x) - 1.0f;
        const float y = 1.0f - (2.0f * pixel.y) / float(viewportSize.y);

        const float aspect = float(viewportSize.x) / float(viewportSize.y);
        const glm::mat4 invViewProj = glm::inverse(viewProjection(aspect));

        const glm::vec4 farClip{x, y, 1.0f, 1.0f};
        glm::vec4 farWorld = invViewProj * farClip;
        farWorld /= farWorld.w;

        Ray ray;
        ray.origin = transform().position;
        ray.direction = glm::normalize(glm::vec3(farWorld) - ray.origin);
        return ray;
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