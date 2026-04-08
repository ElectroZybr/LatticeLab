#include "WindowEvents.h"

#include <imgui-SFML.h>

#include "GUI/interface/interface.h"

sf::RenderWindow* WindowEvents::window = nullptr;
sf::View* WindowEvents::gameView = nullptr;
Interface* WindowEvents::appInterface = nullptr;

void WindowEvents::init(sf::RenderWindow& w, sf::View& sceneView, Interface& appInterface) {
    window = &w;
    gameView = &sceneView;
    WindowEvents::appInterface = &appInterface;
}

void WindowEvents::onEvent(const sf::Event& event) {
    if (event.is<sf::Event::Closed>()) {
        window->close();
    }

    if (const auto* e = event.getIf<sf::Event::Resized>()) {
        gameView->setSize(sf::Vector2f(e->size));
        gameView->setCenter(sf::Vector2f(e->size) / 2.f);
        if (appInterface == nullptr) {
            return;
        }

        appInterface->styleManager.onResize(e->size);
        if (appInterface->fontManager.load(appInterface->styleManager.getScale())) {
            if (!ImGui::SFML::UpdateFontTexture()) {
                // Keep current font pointers if texture update failed.
            }
        }
    }
}
