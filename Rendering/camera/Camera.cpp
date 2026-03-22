#include <cmath>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

Camera::Camera(sf::View* view, float moveSpeed, float zoomSpeed) 
    : view(view), position(0, 0), zoom(20.f), speed(moveSpeed / 20.f), moveSpeed(moveSpeed), zoomSpeed(zoomSpeed),
        isDragging(false), lastMousePos(0, 0) {}
    
void Camera::update(float deltaTime, sf::RenderTarget& target) {
    view->setCenter(position);
    view->setSize(sf::Vector2f(target.getSize()) / zoom);
}

void Camera::move(float offsetX, float offsetY) {
    position.x += offsetX;
    position.y += offsetY;
}

void Camera::setPosition(float x, float y) {
    position.x = x;
    position.y = y;
}

sf::Vector2f Camera::getPosition() const {
    return position;
}

void Camera::zoomAt(float factor, sf::Vector2f mousePos, sf::RenderWindow& window) {
    // Изменяем уровень зума с учетом направления к курсору
    float prevZoom = zoom;
    zoom *= (1.f + factor * zoomSpeed);
    zoom = std::clamp(zoom, 1.f, 500.f);

    speed = moveSpeed / zoom;

    // Плавное следование за указателем мыши при зуме
    if (zoom > 1.f && zoom < 500.f) {
        sf::Vector2i deltaPos = sf::Vector2i(window.getSize()) / 2 - sf::Mouse::getPosition(window);
        position += sf::Vector2f(deltaPos) * 0.1f / zoom * factor;
    }
}

void Camera::orbitDrag(sf::Vector2i delta) {
    constexpr float sensitivity = 0.005f;
    azimuth   -= delta.x * sensitivity;
    elevation += delta.y * sensitivity;
    elevation = std::clamp(elevation, -1.5f, 1.5f);
}

float Camera::getZoom() const {
    return zoom;
}

void Camera::setZoom(float new_zoom) {
    zoom = std::clamp(new_zoom, 1.f, 500.f);
    speed = moveSpeed / zoom;
}

glm::vec3 Camera::getEyePosition() const {
    const float r = moveSpeed / zoom;
    return glm::vec3(
        r * std::cos(elevation) * std::sin(azimuth),
        r * std::sin(elevation),
        r * std::cos(elevation) * std::cos(azimuth)
    );
}

glm::mat4 Camera::getViewMatrix() const {
    // камера всегда смотрит в центр мира
    glm::vec3 eye = getEyePosition();
    return glm::lookAt(eye, glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));

}
