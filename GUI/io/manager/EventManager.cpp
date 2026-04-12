#include "EventManager.h"

#include <imgui.h>

#include "Engine/Simulation.h"
#include "GUI/io/keyboard/Keyboard.h"
#include "GUI/io/mouse/Mouse.h"
#include "GUI/io/window_events/WindowEvents.h"

namespace {
    static ImGuiKey sfKeyToImGui(sf::Keyboard::Key key) {
        switch (key) {
        case sf::Keyboard::Key::Tab:
            return ImGuiKey_Tab;
        case sf::Keyboard::Key::Left:
            return ImGuiKey_LeftArrow;
        case sf::Keyboard::Key::Right:
            return ImGuiKey_RightArrow;
        case sf::Keyboard::Key::Up:
            return ImGuiKey_UpArrow;
        case sf::Keyboard::Key::Down:
            return ImGuiKey_DownArrow;
        case sf::Keyboard::Key::PageUp:
            return ImGuiKey_PageUp;
        case sf::Keyboard::Key::PageDown:
            return ImGuiKey_PageDown;
        case sf::Keyboard::Key::Home:
            return ImGuiKey_Home;
        case sf::Keyboard::Key::End:
            return ImGuiKey_End;
        case sf::Keyboard::Key::Insert:
            return ImGuiKey_Insert;
        case sf::Keyboard::Key::Delete:
            return ImGuiKey_Delete;
        case sf::Keyboard::Key::Backspace:
            return ImGuiKey_Backspace;
        case sf::Keyboard::Key::Space:
            return ImGuiKey_Space;
        case sf::Keyboard::Key::Enter:
            return ImGuiKey_Enter;
        case sf::Keyboard::Key::Escape:
            return ImGuiKey_Escape;
        case sf::Keyboard::Key::LControl:
            return ImGuiKey_LeftCtrl;
        case sf::Keyboard::Key::LShift:
            return ImGuiKey_LeftShift;
        case sf::Keyboard::Key::LAlt:
            return ImGuiKey_LeftAlt;
        case sf::Keyboard::Key::LSystem:
            return ImGuiKey_LeftSuper;
        case sf::Keyboard::Key::RControl:
            return ImGuiKey_RightCtrl;
        case sf::Keyboard::Key::RShift:
            return ImGuiKey_RightShift;
        case sf::Keyboard::Key::RAlt:
            return ImGuiKey_RightAlt;
        case sf::Keyboard::Key::RSystem:
            return ImGuiKey_RightSuper;
        case sf::Keyboard::Key::A:
            return ImGuiKey_A;
        case sf::Keyboard::Key::C:
            return ImGuiKey_C;
        case sf::Keyboard::Key::V:
            return ImGuiKey_V;
        case sf::Keyboard::Key::X:
            return ImGuiKey_X;
        case sf::Keyboard::Key::Y:
            return ImGuiKey_Y;
        case sf::Keyboard::Key::Z:
            return ImGuiKey_Z;
        default:
            return ImGuiKey_None;
        }
    }
}

sf::RenderWindow* EventManager::window = nullptr;
std::unique_ptr<IRenderer>* EventManager::renderer = nullptr;

void EventManager::init(sf::RenderWindow& w, sf::View& sceneView, Simulation& simulation, std::unique_ptr<IRenderer>& r,
                        Interface& appInterface) {
    window = &w;
    renderer = &r;

    WindowEvents::init(w, sceneView, appInterface);
    Keyboard::init(r, appInterface);
    Mouse::init(w, r, simulation, appInterface);
}

void EventManager::poll() {
    while (const std::optional<sf::Event> optEvent = window->pollEvent()) {
        const sf::Event& event = *optEvent;

        ImGuiIO& io = ImGui::GetIO();

        if (const auto* e = event.getIf<sf::Event::MouseMoved>()) {
            io.AddMousePosEvent(float(e->position.x), float(e->position.y));
        }
        if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
            io.AddMouseButtonEvent(e->button == sf::Mouse::Button::Left ? 0 : e->button == sf::Mouse::Button::Right ? 1 : 2, true);
        }
        if (const auto* e = event.getIf<sf::Event::MouseButtonReleased>()) {
            io.AddMouseButtonEvent(e->button == sf::Mouse::Button::Left ? 0 : e->button == sf::Mouse::Button::Right ? 1 : 2, false);
        }
        if (const auto* e = event.getIf<sf::Event::MouseWheelScrolled>()) {
            io.AddMouseWheelEvent(0.f, e->delta);
        }
        if (const auto* e = event.getIf<sf::Event::TextEntered>()) {
            io.AddInputCharacter(e->unicode);
        }
        if (const auto* e = event.getIf<sf::Event::KeyPressed>()) {
            io.AddKeyEvent(sfKeyToImGui(e->code), true);
        }
        if (const auto* e = event.getIf<sf::Event::KeyReleased>()) {
            io.AddKeyEvent(sfKeyToImGui(e->code), false);
        }

        WindowEvents::onEvent(event);
        Keyboard::onEvent(event);
        Mouse::onEvent(event);
    }
}

void EventManager::frame(float deltaTime) {
    Keyboard::onFrame(deltaTime);
    Mouse::onFrame(deltaTime);
}
