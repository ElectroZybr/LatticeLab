#include "Keyboard.h"
#include "GUI/interface/interface.h"

std::unique_ptr<IRenderer>* Keyboard::render = nullptr;
std::optional<KeyboardCommand> Keyboard::pendingResult = std::nullopt;

void Keyboard::init(std::unique_ptr<IRenderer>& r) {
    render = &r;
}

void Keyboard::onEvent(const sf::Event& event) {
    if (const auto* e = event.getIf<sf::Event::KeyPressed>()) {
        if (e->code == sf::Keyboard::Key::P)
            Interface::debugPanel.toggle();
        else if (e->code == sf::Keyboard::Key::Space)
            Interface::pause = !Interface::pause;
        else if (e->code == sf::Keyboard::Key::Right && Interface::getPause())
        {
            pendingResult = KeyboardCommand::StepPhysics;
        }
    }
}

void Keyboard::onFrame(float deltaTime) {
    std::unique_ptr<IRenderer>& rend = *render;
    if (rend->camera.mode == Camera::Mode::Orbit) {
        float rotSpeed = 1.5f * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            rend->camera.elevation = std::clamp(rend->camera.elevation + rotSpeed, -1.5f, 1.5f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            rend->camera.elevation = std::clamp(rend->camera.elevation - rotSpeed, -1.5f, 1.5f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) rend->camera.azimuth -= rotSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) rend->camera.azimuth += rotSpeed;
    }
    else if (rend->camera.mode == Camera::Mode::Free) {
        const glm::vec3 forward(
            std::cos(rend->camera.elevation) * std::sin(rend->camera.azimuth),
            std::sin(rend->camera.elevation),
            std::cos(rend->camera.elevation) * std::cos(rend->camera.azimuth)
        );
        const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.f, 1.f, 0.f)));
        const float s = rend->camera.speed * deltaTime;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) rend->camera.move3D(Vec3f( forward.x,  forward.y,  forward.z) * s);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) rend->camera.move3D(Vec3f(-forward.x, -forward.y, -forward.z) * s);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) rend->camera.move3D(Vec3f(-right.x,   -right.y,   -right.z)   * s);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) rend->camera.move3D(Vec3f( right.x,    right.y,    right.z)   * s);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)) rend->camera.move3D(Vec3f(0.f, -s, 0.f));
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) rend->camera.move3D(Vec3f(0.f,  s, 0.f));

    }
    else if (rend->camera.mode == Camera::Mode::Mode2D) {
        float deltaSpeed = rend->camera.speed * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) rend->camera.move({0,  deltaSpeed});
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) rend->camera.move({0, -deltaSpeed});
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) rend->camera.move({-deltaSpeed, 0});
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) rend->camera.move({ deltaSpeed, 0});
    }
}

std::optional<KeyboardCommand> Keyboard::popResult() {
    auto result = pendingResult;
    pendingResult = std::nullopt;
    return result;
}
