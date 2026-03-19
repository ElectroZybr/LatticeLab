#include "EventManager.h"
#include "imgui-SFML.h"

#include "Engine/io/mouse/Mouse.h"
#include "Engine/io/keyboard/Keyboard.h"
#include "Engine/io/window_events/WindowEvents.h"

sf::RenderWindow* EventManager::window = nullptr;
IRenderer*        EventManager::render = nullptr;

void EventManager::init(sf::RenderWindow* w, sf::View* ui, IRenderer* r, SimBox* b, std::vector<Atom>* a) {
    window = w;
    render = r;

    WindowEvents::init(w, ui);
    Keyboard::init(r);
    Mouse::init(w, r, b, a);
}

void EventManager::updateRenderer(IRenderer* r) {
    render = r;
    Keyboard::init(r);
    Mouse::init(window, r, Mouse::box, Mouse::atoms);
}

void EventManager::poll() {
    while (const std::optional<sf::Event> optEvent = window->pollEvent()) {
        const sf::Event& event = *optEvent;
        ImGui::SFML::ProcessEvent(*window, event);

        WindowEvents::onEvent(event);
        Keyboard::onEvent(event);
        Mouse::onEvent(event);
    }
}

void EventManager::frame(float deltaTime) {
    Keyboard::onFrame(deltaTime);
    Mouse::onFrame();
}
