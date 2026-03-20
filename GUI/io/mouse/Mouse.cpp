#include "GUI/io/mouse/Mouse.h"

#include "Engine/Tools.h"
#include "GUI/interface/interface.h"

sf::RenderWindow*  Mouse::window = nullptr;
IRenderer*         Mouse::render = nullptr;
SimBox*            Mouse::box = nullptr;
std::vector<Atom>* Mouse::atoms = nullptr;

void Mouse::init(sf::RenderWindow* w, IRenderer* r, SimBox* b, std::vector<Atom>* a) {
    window = w;
    render = r;
    box = b;
    atoms = a;
}

void Mouse::onEvent(const sf::Event& event) {
    const sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);

    if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Left) {
            Tools::onLeftPressed(mouse_pos, *atoms);
        }

        if (e->button == sf::Mouse::Button::Right && !Interface::cursorHovered) {
            render->camera.isDragging = true;
            render->camera.dragStartPixelPos = mouse_pos;
            render->camera.dragStartCameraPos = render->camera.position;
        }
    }

    if (const auto* e = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (e->button == sf::Mouse::Button::Left) {
            Tools::onLeftReleased(*atoms);
        }

        if (e->button == sf::Mouse::Button::Right) {
            render->camera.isDragging = false;
        }
    }

    if (event.getIf<sf::Event::MouseMoved>() && render->camera.isDragging) {
        const sf::Vector2i currentPixelPos = sf::Mouse::getPosition(*window);
        const sf::Vector2i deltaPixel = currentPixelPos - render->camera.dragStartPixelPos;

        if (render->camera.orbitMode) {
            render->camera.orbitDrag(deltaPixel);
            render->camera.dragStartPixelPos = currentPixelPos;
        }
        else {
            const sf::Vector2f deltaWorld = window->mapPixelToCoords(render->camera.dragStartPixelPos, *render->camera.view)
                - window->mapPixelToCoords(currentPixelPos, *render->camera.view);
            render->camera.position = render->camera.dragStartCameraPos + deltaWorld;
        }
    }

    if (const auto* e = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (e->wheel == sf::Mouse::Wheel::Vertical) {
            render->camera.zoomAt(e->delta, sf::Vector2f(e->position), *window);
        }
    }
}

void Mouse::onFrame() {
    Tools::onFrame(*atoms);
}
