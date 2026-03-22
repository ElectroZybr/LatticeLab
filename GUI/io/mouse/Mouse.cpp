#include <iostream>

#include "GUI/io/mouse/Mouse.h"

#include "Engine/Tools.h"
#include "GUI/interface/interface.h"

sf::RenderWindow*  Mouse::window = nullptr;
std::unique_ptr<IRenderer>* Mouse::renderer = nullptr;
SimBox* Mouse::box = nullptr;
std::vector<Atom>* Mouse::atoms = nullptr;

void Mouse::init(sf::RenderWindow* w, std::unique_ptr<IRenderer>& r, SimBox* b, std::vector<Atom>* a) {
    window = w;
    renderer = &r;
    box = b;
    atoms = a;
}

void Mouse::onEvent(const sf::Event& event) {
    const sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);
    std::unique_ptr<IRenderer>& rend = *renderer;

    if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Left) {
            Tools::onLeftPressed(mouse_pos, *atoms);
        }

        if (e->button == sf::Mouse::Button::Right && !Interface::cursorHovered) {
            rend->camera.isDragging = true;
            rend->camera.dragStartPixelPos = mouse_pos;
            rend->camera.dragStartCameraPos = rend->camera.position;
        }
    }

    if (const auto* e = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (e->button == sf::Mouse::Button::Left) {
            Tools::onLeftReleased(*atoms);
        }

        if (e->button == sf::Mouse::Button::Right) {
            rend->camera.isDragging = false;
        }
    }

    if (event.getIf<sf::Event::MouseMoved>() && rend->camera.isDragging) {
        const sf::Vector2i currentPixelPos = sf::Mouse::getPosition(*window);
        const sf::Vector2i deltaPixel = currentPixelPos - rend->camera.dragStartPixelPos;

        if (rend->camera.orbitMode) {
            rend->camera.orbitDrag(deltaPixel);
            rend->camera.dragStartPixelPos = currentPixelPos;
        }
        else {
            const sf::Vector2f deltaWorld = window->mapPixelToCoords(rend->camera.dragStartPixelPos, *rend->camera.view)
                - window->mapPixelToCoords(currentPixelPos, *rend->camera.view);
            rend->camera.position = rend->camera.dragStartCameraPos + deltaWorld;
        }
    }

    if (const auto* e = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (e->wheel == sf::Mouse::Wheel::Vertical) {
            rend->camera.zoomAt(e->delta, sf::Vector2f(e->position), *window);
        }
    }
}

void Mouse::onFrame() {
    Tools::onFrame(*atoms);
}

void Mouse::logMousePos() {
    sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);
    Vec2D world_pos = Tools::screenToWorld(mouse_pos, (*renderer)->camera.getZoom());
    Vec2D local_pos(world_pos.x - box->start.x, world_pos.y - box->start.y);
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