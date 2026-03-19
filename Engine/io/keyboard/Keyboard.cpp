#include "Keyboard.h"
#include "GUI/interface/interface.h"

IRenderer* Keyboard::render = nullptr;

void Keyboard::init(IRenderer* r) {
    render = r;
}

void Keyboard::onEvent(const sf::Event& event) {
    if (const auto* e = event.getIf<sf::Event::KeyPressed>()) {
        if (e->code == sf::Keyboard::Key::P)
            Interface::debugPanel.toggle();
        else if (e->code == sf::Keyboard::Key::Space)
            Interface::pause = !Interface::pause;
    }
}

void Keyboard::onFrame(float deltaTime) {
    if (!render->camera.orbitMode) {
        float deltaSpeed = render->camera.speed * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) render->camera.move(0, -deltaSpeed);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) render->camera.move(0,  deltaSpeed);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) render->camera.move(-deltaSpeed, 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) render->camera.move( deltaSpeed, 0);
    } else {
        float rotSpeed = 1.5f * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            render->camera.elevation = std::clamp(render->camera.elevation + rotSpeed, -1.5f, 1.5f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            render->camera.elevation = std::clamp(render->camera.elevation - rotSpeed, -1.5f, 1.5f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) render->camera.azimuth -= rotSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) render->camera.azimuth += rotSpeed;
    }
}