#include "Mouse.h"

#include <cmath>
#include <iostream>

#include "App/interaction/ToolsManager.h"
#include "Engine/Simulation.h"
#include "GUI/interface/interface.h"

sf::RenderWindow* Mouse::window = nullptr;
std::unique_ptr<IRenderer>* Mouse::renderer = nullptr;
Interface* Mouse::appInterface = nullptr;

void Mouse::init(sf::RenderWindow& w, std::unique_ptr<IRenderer>& r, Simulation& simulation, Interface& appInterface) {
    window = &w;
    renderer = &r;
    Mouse::appInterface = &appInterface;
    (void)simulation;
}

void Mouse::onEvent(const sf::Event& event) {
    const sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);
    std::unique_ptr<IRenderer>& rend = *renderer;
    constexpr float kFreeWheelMoveScale = 0.008f;

    if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Left) {
            ToolsManager::onLeftPressed(mouse_pos);
        }

        if (e->button == sf::Mouse::Button::Right && appInterface != nullptr && !appInterface->state().cursorHovered &&
            !ToolsManager::isInteractingNow()) {
            if (!ToolsManager::onRightPressed(mouse_pos)) {
                rend->camera.isDragging = true;
                rend->camera.dragStartPixelPos = mouse_pos;
                rend->camera.dragStartCameraPos = rend->camera.position;
            }
        }
    }

    if (const auto* e = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (e->button == sf::Mouse::Button::Left) {
            ToolsManager::onLeftReleased(mouse_pos);
        }

        if (e->button == sf::Mouse::Button::Right) {
            rend->camera.isDragging = false;
        }
    }

    if (event.getIf<sf::Event::MouseMoved>() && rend->camera.isDragging) {
        const sf::Vector2i currentPixelPos = sf::Mouse::getPosition(*window);
        sf::Vector2i deltaPixel = currentPixelPos - rend->camera.dragStartPixelPos;

        if (rend->camera.mode == Camera::Mode::Orbit) {
            rend->camera.orbitDrag(deltaPixel);
            rend->camera.dragStartPixelPos = currentPixelPos;
        }
        else if (rend->camera.mode == Camera::Mode::Free) {
            rend->camera.freeDrag(deltaPixel);
            rend->camera.dragStartPixelPos = currentPixelPos;
        }
        else {
            Vec3f deltaWorld = ToolsManager::screenToWorld(rend->camera.dragStartPixelPos) - ToolsManager::screenToWorld(currentPixelPos);
            rend->camera.position = rend->camera.dragStartCameraPos + Vec2f(deltaWorld.x, deltaWorld.y);
        }
    }

    if (const auto* e = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (e->wheel == sf::Mouse::Wheel::Vertical && appInterface != nullptr && !appInterface->state().cursorHovered &&
            !ToolsManager::isInteractingNow()) {
            if (rend->camera.getMode() == Camera::Mode::Free) {
                const float wheelStep = rend->camera.moveSpeed * kFreeWheelMoveScale;
                const float distance = e->delta * wheelStep;

                const Vec3f forward(std::cos(rend->camera.elevation) * std::sin(rend->camera.azimuth), std::sin(rend->camera.elevation),
                                    std::cos(rend->camera.elevation) * std::cos(rend->camera.azimuth));
                rend->camera.move3D(forward * distance);
            }
            else {
                rend->camera.zoomAt(e->delta, sf::Vector2f(e->position), *window);
            }
        }
    }
}

void Mouse::onFrame(float deltaTime) {
    const sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);
    ToolsManager::onFrame(mouse_pos, deltaTime);
}

void Mouse::logMousePos() {
    sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);
    Vec3f world_pos = ToolsManager::screenToWorld(mouse_pos);
    std::cout << "<Mouse pos>"
              << " Screen: "
              << "X " << mouse_pos.x << "Y " << mouse_pos.y << " | World: "
              << "X " << world_pos.x << "Y " << world_pos.y << std::endl;
}
