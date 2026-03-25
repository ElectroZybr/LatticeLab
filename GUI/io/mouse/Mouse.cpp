#include <iostream>

#include "GUI/io/mouse/Mouse.h"

#include "Engine/Tools.h"
#include "GUI/interface/interface.h"

sf::RenderWindow*  Mouse::window = nullptr;
std::unique_ptr<IRenderer>* Mouse::renderer = nullptr;
SimBox* Mouse::box = nullptr;
AtomStorage* Mouse::atomStorage = nullptr;

void Mouse::init(sf::RenderWindow* w, std::unique_ptr<IRenderer>& r, SimBox* b, AtomStorage* storage) {
    window = w;
    renderer = &r;
    box = b;
    atomStorage = storage;
}

void Mouse::onEvent(const sf::Event& event) {
    const sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);
    std::unique_ptr<IRenderer>& rend = *renderer;

    if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Left) {
            Tools::onLeftPressed(mouse_pos);
        }

        if (e->button == sf::Mouse::Button::Right && !Interface::cursorHovered) {
            rend->camera.isDragging = true;
            rend->camera.dragStartPixelPos = mouse_pos;
            rend->camera.dragStartCameraPos = rend->camera.position;
        }
    }

    if (const auto* e = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (e->button == sf::Mouse::Button::Left) {
            Tools::onLeftReleased();
        }

        if (e->button == sf::Mouse::Button::Right) {
            rend->camera.isDragging = false;
        }
    }

    if (event.getIf<sf::Event::MouseMoved>() && rend->camera.isDragging) {
        const sf::Vector2i currentPixelPos = sf::Mouse::getPosition(*window);
        sf::Vector2i deltaPixel = currentPixelPos - rend->camera.dragStartPixelPos;

        if (rend->camera.orbitMode) {
            rend->camera.orbitDrag(deltaPixel);
            rend->camera.dragStartPixelPos = currentPixelPos;
        }
        else {
            sf::Vector2f deltaWorld = Tools::screenToWorld(rend->camera.dragStartPixelPos) 
                                    - Tools::screenToWorld(currentPixelPos);
            rend->camera.position = rend->camera.dragStartCameraPos + deltaWorld;
        }
    }

    if (const auto* e = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (e->wheel == sf::Mouse::Wheel::Vertical) {
            rend->camera.zoomAt(e->delta, sf::Vector2f(e->position), *window);
        }
    }
}

void Mouse::onFrame(float deltaTime) {
    Tools::onFrame(deltaTime);
}

void Mouse::logMousePos() {
    sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);
    Vec2f world_pos = Tools::screenToWorld(mouse_pos);
    Vec2f local_pos = Tools::screenToBox(mouse_pos);
    std::cout << "<Mouse pos>"
              << " Screen: "
              << "X " << mouse_pos.x
              << " Y " << mouse_pos.y
              << " | World: "
              << "X " << world_pos.x
              << " Y " << world_pos.y
              << " | Local: "
              << "X " << local_pos.x
              << " Y " << local_pos.y
            << std::endl;
}
