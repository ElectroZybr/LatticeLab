#include "EventManager.h"

#include <imgui-SFML.h>

#include "Engine/Simulation.h"
#include "GUI/io/keyboard/Keyboard.h"
#include "GUI/io/mouse/Mouse.h"
#include "GUI/io/window_events/WindowEvents.h"

sf::RenderWindow* EventManager::window = nullptr;
std::unique_ptr<IRenderer>* EventManager::renderer = nullptr;

void EventManager::init(sf::RenderWindow& w, sf::View& uiView, Simulation& simulation, std::unique_ptr<IRenderer>& r, Interface& ui) {
    window = &w;
    renderer = &r;

    WindowEvents::init(w, uiView, ui);
    Keyboard::init(r, ui);
    Mouse::init(w, r, simulation, ui);
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
    Mouse::onFrame(deltaTime);
}
