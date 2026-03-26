#include <cmath>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

Camera::Camera(sf::View* view, float moveSpeed, float zoomSpeed) 
    : view(view), position(0, 0), zoom(20.f), speed(moveSpeed / 20.f), moveSpeed(moveSpeed), zoomSpeed(zoomSpeed),
    isDragging(false), lastMousePos(0, 0) {
}

void Camera::update(sf::RenderTarget& target) {
    screenSize = sf::Vector2f(target.getSize());
    view->setCenter(position);
    view->setSize((screenSize / zoom).componentWiseMul({1.f, -1.f}));
}

void Camera::setZoom(float new_zoom) {
    zoom = std::clamp(new_zoom, 1.f, 500.f);
    speed = moveSpeed / zoom;
}

void Camera::zoomAt(float factor, sf::Vector2f mousePos, sf::RenderWindow& window) {
    // Изменяем уровень зума с учетом направления к курсору
    zoom *= (1.f + factor * zoomSpeed);
    zoom = std::clamp(zoom, 1.f, 500.f);
    speed = moveSpeed / zoom;

    // Плавное следование за указателем мыши при зуме
    if (zoom > 1.f && zoom < 500.f) {
        sf::Vector2i deltaPos = sf::Mouse::getPosition(window) - sf::Vector2i(screenSize) / 2;
        deltaPos.y *= -1;
        position += Vec2f(deltaPos.x, deltaPos.y) * 0.1f / zoom * factor;
    }
}

void Camera::orbitDrag(sf::Vector2i delta) {
    constexpr float sensitivity = 0.005f;
    azimuth   -= delta.x * sensitivity;
    elevation += delta.y * sensitivity;
    elevation = std::clamp(elevation, -1.5f, 1.5f);
}

void Camera::freeDrag(sf::Vector2i delta) {
    constexpr float sensitivity = 0.005f;
    azimuth   -= delta.x * sensitivity;
    elevation += delta.y * sensitivity;
    elevation = std::clamp(elevation, -1.5f, 1.5f);
}

// Для 3д режимов возвращает cameraPos + cameraDir * 10
Vec3f Camera::screenToWorld(sf::Vector2i screenPos) const {
    if (mode == Mode::Mode2D) {
        const sf::Vector2f viewSize   = view->getSize();
        const sf::Vector2f viewCenter = view->getCenter();
        const float wx = viewCenter.x + (screenPos.x - screenSize.x * 0.5f) * (viewSize.x / screenSize.x);
        const float wy = viewCenter.y + (screenPos.y - screenSize.y * 0.5f) * (viewSize.y / screenSize.y);
        return Vec3f(wx, wy, 0.f);
    }
 
    const Ray ray = screenToRay(screenPos.x, screenPos.y);
    return ray.at(10.0);
}

sf::Vector2i Camera::worldToScreen(Vec3f worldPos) const {
    if (mode == Mode::Mode2D) {
        const sf::Vector2f viewSize   = view->getSize();
        const sf::Vector2f viewCenter = view->getCenter();
        const float sx = (worldPos.x - viewCenter.x) * (screenSize.x / viewSize.x) + screenSize.x * 0.5f;
        const float sy = (worldPos.y - viewCenter.y) * (screenSize.y / viewSize.y) + screenSize.y * 0.5f;
        return sf::Vector2i(static_cast<int>(sx), static_cast<int>(sy));
    }

    const glm::vec4 clip = getProjectionMatrix()
                         * getViewMatrix()
                         * glm::vec4(worldPos.x, worldPos.y, worldPos.z, 1.f);

    if (clip.w <= 0.f) return { -1, -1 };

    const float ndcX = clip.x / clip.w;
    const float ndcY = clip.y / clip.w;

    return sf::Vector2i(
        static_cast<int>(( ndcX + 1.f) * 0.5f * screenSize.x),
        static_cast<int>((-ndcY + 1.f) * 0.5f * screenSize.y)
    );
}

glm::vec3 Camera::getEyePosition() const {
    const float r = moveSpeed / zoom;
    return r * glm::vec3(
        std::cos(elevation) * std::sin(azimuth),
        std::sin(elevation),
        std::cos(elevation) * std::cos(azimuth)
    );
}

glm::mat4 Camera::getViewMatrix() const {
    // камера всегда смотрит в центр мира
    glm::vec3 eye = getEyePosition();
    return glm::lookAt(eye, glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));

}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(
        glm::radians(FOV),
        screenSize.x / screenSize.y,
        NEAR,
        FAR
    );
}

Ray Camera::screenToRay(float screenX, float screenY) const
{
    const float ndcX = (screenX / screenSize.x)  * 2.f - 1.f;
    const float ndcY = 1.f - (screenY / screenSize.y) * 2.f;
 
    const glm::vec4 rayClip(ndcX, ndcY, -1.f, 1.f);
 
    glm::vec4 rayEye = glm::inverse(getProjectionMatrix()) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.f, 0.f);
 
    const glm::vec3 rayDirGLM = glm::normalize(
        glm::vec3(glm::inverse(getViewMatrix()) * rayEye)
    );
 
    const glm::vec3 eye = getEyePosition();
 
    return Ray(
        Vec3f(eye.x, eye.y, eye.z),
        Vec3f(rayDirGLM.x, rayDirGLM.y, rayDirGLM.z)
    );
}
