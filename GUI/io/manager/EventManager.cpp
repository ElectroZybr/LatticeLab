#include "EventManager.h"
#include "imgui-SFML.h"

#include "GUI/io/mouse/Mouse.h"
#include "GUI/io/keyboard/Keyboard.h"
#include "GUI/io/window_events/WindowEvents.h"

sf::RenderWindow* EventManager::window = nullptr;
std::unique_ptr<IRenderer>* EventManager::renderer = nullptr;

void EventManager::init(sf::RenderWindow* w, sf::View* ui, std::unique_ptr<IRenderer>& r, SimBox* b, AtomStorage* atomStorage) {
    window = w;
    renderer = &r;

    WindowEvents::init(w, ui);
    Keyboard::init(r);
    Mouse::init(w, r, b, atomStorage);
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
