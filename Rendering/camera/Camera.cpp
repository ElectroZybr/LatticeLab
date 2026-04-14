#include "Camera.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "Engine/SimBox.h"

Camera::Camera(SimBox& simBox, float moveSpeed, float zoomSpeed)
    : simBox(simBox), moveSpeed(moveSpeed), zoomSpeed(zoomSpeed), isDragging(false), lastMousePos(0, 0) {}

void Camera::setZoom(float new_zoom) {
    zoom = std::clamp(new_zoom, 1.f, 500.f);
    speed = moveSpeed / zoom;
}

void Camera::zoomAt(float factor, Vec2f mousePos) {
    // Изменяем уровень зума с учетом направления к курсору
    zoom *= (1.f + factor * zoomSpeed);
    zoom = std::clamp(zoom, 1.f, 500.f);
    speed = moveSpeed / zoom;

    // Плавное следование за указателем мыши при зуме
    if (zoom > 1.f && zoom < 500.f) {
        Vec2f deltaPos(mousePos - screenSize / 2.f);
        deltaPos.y *= -1.f;
        position += deltaPos * 0.1f / zoom * factor;
    }
}

void Camera::orbitDrag(Vec2i delta) {
    constexpr float sensitivity = 0.005f;
    azimuth -= delta.x * sensitivity;
    elevation += delta.y * sensitivity;
    elevation = std::clamp(elevation, -1.5f, 1.5f);
}

void Camera::freeDrag(Vec2i delta) {
    constexpr float sensitivity = 0.003f;
    azimuth -= delta.x * sensitivity;
    elevation -= delta.y * sensitivity;
    elevation = std::clamp(elevation, -1.5f, 1.5f);
}

// Для 3д режимов возвращает cameraPos + cameraDir * 10
Vec3f Camera::screenToWorld(Vec2i screenPos) const {
    if (mode == Mode::Mode2D) {
        Vec2f offset = Vec2f(screenPos) - screenSize * 0.5f;
        offset.y *= -1.f;
        const Vec2f w = position + offset / zoom;
        return Vec3f(w);
    }

    const Ray ray = screenToRay(screenPos.x, screenPos.y);
    return ray.at(100.0);
}

Vec2i Camera::worldToScreen(Vec3f worldPos) const {
    if (mode == Mode::Mode2D) {
        Vec2f offset = worldPos.xy() - position;
        offset.y *= -1.f;
        const Vec2f s = offset * zoom + screenSize * 0.5f;
        return Vec2i(s);
    }

    const glm::vec4 clip = getProjectionMatrix() * getViewMatrix() * glm::vec4(worldPos.x, worldPos.y, worldPos.z, 1.f);

    if (clip.w <= 0.f) {
        return {-1, -1};
    }

    const float ndcX = clip.x / clip.w;
    const float ndcY = clip.y / clip.w;

    return Vec2i(static_cast<int>((ndcX + 1.f) * 0.5f * screenSize.x), static_cast<int>((-ndcY + 1.f) * 0.5f * screenSize.y));
}

glm::vec3 Camera::getEyePosition() const {
    if (mode == Mode::Free) {
        return glm::vec3(freePosition.x, freePosition.y, freePosition.z);
    }

    const Vec3f center = simBox.size / 2.f;
    const glm::vec3 glmCenter(center.x, center.y, center.z);
    const float r = moveSpeed / zoom;
    return glmCenter + r * glm::vec3(std::cos(elevation) * std::sin(azimuth), std::sin(elevation), std::cos(elevation) * std::cos(azimuth));
}

glm::mat4 Camera::getViewMatrix() const {
    if (mode == Mode::Free) {
        const glm::vec3 forward(std::cos(elevation) * std::sin(azimuth), std::sin(elevation), std::cos(elevation) * std::cos(azimuth));
        const glm::vec3 eye(freePosition.x, freePosition.y, freePosition.z);
        return glm::lookAt(eye, eye + forward, glm::vec3(0.f, 1.f, 0.f));
    }

    // камера всегда смотрит в центр коробки
    Vec3f center = simBox.size / 2.f;
    glm::vec3 eye = getEyePosition();
    return glm::lookAt(eye, glm::vec3(center.x, center.y, center.z), glm::vec3(0.f, 1.f, 0.f));
}

glm::mat4 Camera::getProjectionMatrix() const {
    const float fov = (mode == Mode::Free) ? FOV_FREE : FOV_ORBIT;
    return glm::perspective(glm::radians(fov), screenSize.x / screenSize.y, NEAR, FAR);
}

Ray Camera::screenToRay(float screenX, float screenY) const {
    const float ndcX = (screenX / screenSize.x) * 2.f - 1.f;
    const float ndcY = 1.f - (screenY / screenSize.y) * 2.f;

    const glm::vec4 rayClip(ndcX, ndcY, -1.f, 1.f);

    glm::vec4 rayEye = glm::inverse(getProjectionMatrix()) * rayClip;
    rayEye = glm::vec4(rayEye.x / rayEye.w, rayEye.y / rayEye.w, -1.f, 0.f);

    const glm::vec3 rayDirGLM = glm::normalize(glm::vec3(glm::inverse(getViewMatrix()) * rayEye));

    const glm::vec3 eye = getEyePosition();

    return Ray(Vec3f(eye.x, eye.y, eye.z), Vec3f(rayDirGLM.x, rayDirGLM.y, rayDirGLM.z));
}
