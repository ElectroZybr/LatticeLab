#include <cmath>
#include <algorithm>
#include "Camera.h"
#include "../interface.h"

Camera::Camera(sf::RenderWindow& window, sf::View* view, float moveSpeed, float zoomSpeed) 
    : view(view), position(0, 0), zoom(20.f), speed(moveSpeed / 20.f), moveSpeed(moveSpeed), zoomSpeed(zoomSpeed),
        isDragging(false), lastMousePos(0, 0) {}
    
void Camera::update(float deltaTime, sf::RenderWindow& window) {
    view->setCenter(position);
    view->setSize(sf::Vector2f(window.getSize()) / zoom);
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
        sf::Vector2i deltaPos = sf::Mouse::getPosition(window) - sf::Vector2i(window.getSize()) / 2;
        position += sf::Vector2f(deltaPos) * 0.1f / zoom * factor;
    }
}

float Camera::getZoom() const {
    return zoom;
}

void Camera::setZoom(float new_zoom) {
    zoom = std::clamp(new_zoom, 1.f, 500.f);
    speed = moveSpeed / zoom;
}

void Camera::handleInput(float deltaTime, sf::RenderWindow& window) {
    // Управление WASD
    float deltaSpeed = speed * deltaTime;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) move(0, -deltaSpeed);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) move(0, deltaSpeed);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) move(-deltaSpeed, 0);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) move(deltaSpeed, 0);
}

void Camera::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (!Interface::cursorHovered) {
        if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (e->button == sf::Mouse::Button::Right) {
                // Начало перетаскивания
                isDragging = true;
                dragStartPixelPos = sf::Mouse::getPosition(window);
                dragStartCameraPos = position;
            }
        }
    }

    if (const auto* e = event.getIf<sf::Event::MouseMoved>()) {
        if (isDragging) {
            // Перемещение камеры при перетаскивании
            sf::Vector2i currentPixelPos = sf::Mouse::getPosition(window);
            sf::Vector2i deltaPixel = dragStartPixelPos - currentPixelPos;

            // Преобразуем разницу в пикселях в мировые координаты
            sf::Vector2f deltaWorld = window.mapPixelToCoords(
                deltaPixel, *view
            ) - window.mapPixelToCoords(sf::Vector2i(0, 0), *view);

            position = dragStartCameraPos + deltaWorld;
        }
    }
    else if (const auto* e = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (e->button == sf::Mouse::Button::Right) {
            // Конец перетаскивания
            isDragging = false;
        }
    }
    else if (const auto* e = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (e->wheel == sf::Mouse::Wheel::Vertical) {
            zoomAt(e->delta, sf::Vector2f(e->position), window);
        }
    }
}
