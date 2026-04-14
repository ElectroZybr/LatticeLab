#include "WindowEvents.h"

#include "imgui_impl_bgfx.h"

#include "App/AppSignals.h"
#include "GUI/interface/interface.h"

sf::RenderWindow* WindowEvents::window = nullptr;
Interface* WindowEvents::appInterface = nullptr;

void WindowEvents::init(sf::RenderWindow& w, Interface& appInterface) {
    window = &w;
    WindowEvents::appInterface = &appInterface;
}

void WindowEvents::onEvent(const sf::Event& event) {
    if (event.is<sf::Event::Closed>()) {
        window->close();
    }

    if (const auto* e = event.getIf<sf::Event::Resized>()) {
        AppSignals::Window::Resize.emit(Vec2f(e->size));

        if (appInterface == nullptr) {
            return;
        }
        appInterface->styleManager.onResize(Vec2u(e->size));
        if (appInterface->fontManager.load(appInterface->styleManager.getScale())) {
            ImGui_Implbgfx_InvalidateDeviceObjects();
            ImGui_Implbgfx_CreateDeviceObjects();
        }
    }
}
