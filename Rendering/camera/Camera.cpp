#include <cmath>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

Camera::Camera(sf::View* view, float moveSpeed, float zoomSpeed) 
    : view(view), position(0, 0), zoom(20.f), speed(moveSpeed / 20.f), moveSpeed(moveSpeed), zoomSpeed(zoomSpeed),
    isDragging(false), lastMousePos(0, 0) {
}

void Camera::update(sf::RenderTarget& target) {
    view->setCenter(position);
    view->setSize((sf::Vector2f(target.getSize()) / zoom).componentWiseMul({1.f, -1.f}));
}

void Camera::zoomAt(float factor, sf::Vector2f mousePos, sf::RenderWindow& window) {
    // Изменяем уровень зума с учетом направления к курсору
    float prevZoom = zoom;
    zoom *= (1.f + factor * zoomSpeed);
    zoom = std::clamp(zoom, 1.f, 500.f);

    speed = moveSpeed / zoom;

    // Плавное следование за указателем мыши при зуме
    if (zoom > 1.f && zoom < 500.f) {
        sf::Vector2i deltaPos = sf::Mouse::getPosition(window) - sf::Vector2i(window.getSize()) / 2;
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

void Camera::setZoom(float new_zoom) {
    zoom = std::clamp(new_zoom, 1.f, 500.f);
    speed = moveSpeed / zoom;
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

glm::mat4 Camera::getProjectionMatrix(float screenWidth, float screenHeight) const {
    return glm::perspective(
        glm::radians(FOV),
        screenWidth / screenHeight,
        NEAR,
        FAR
    );
}

Ray Camera::screenToRay(float screenX, float screenY, float screenWidth, float screenHeight) const
{
    const float ndcX = (screenX / screenWidth)  * 2.f - 1.f;
    const float ndcY = 1.f - (screenY / screenHeight) * 2.f;
 
    const glm::vec4 rayClip(ndcX, ndcY, -1.f, 1.f);
 
    glm::vec4 rayEye = glm::inverse(getProjectionMatrix(screenWidth, screenHeight)) * rayClip;
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

ScreenPoint Camera::worldToScreen(const Vec3f& worldPos, float screenWidth, float screenHeight) const
{
    const glm::vec4 clip = getProjectionMatrix(screenWidth, screenHeight)
                         * getViewMatrix()
                         * glm::vec4(
                               static_cast<float>(worldPos.x),
                               static_cast<float>(worldPos.y),
                               static_cast<float>(worldPos.z),
                               1.f
                           );
 
    // За near plane — невидимо
    if (clip.w <= 0.f)
        return { Vec2f{0.f, 0.f}, false };
 
    // Перспективное деление → NDC
    const float ndcX = clip.x / clip.w;
    const float ndcY = clip.y / clip.w;
 
    // NDC → пиксели (Y снова переворачиваем)
    return {
        (Vec2f(ndcX,  -ndcY) + 1.f)  * 0.5f * screenHeight,
        true
    };
}
