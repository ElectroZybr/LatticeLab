#include "WindowEvents.h"
#include "GUI/interface/interface.h"
#include "imgui-SFML.h"

sf::RenderWindow* WindowEvents::window = nullptr;
sf::View*         WindowEvents::gameView = nullptr;

void WindowEvents::init(sf::RenderWindow* w, sf::View* gv) {
    window = w;
    gameView = gv;
}

void WindowEvents::onEvent(const sf::Event& event) {
    if (event.is<sf::Event::Closed>())
        window->close();

    if (const auto* e = event.getIf<sf::Event::Resized>()) {
        gameView->setSize(sf::Vector2f(e->size));
        gameView->setCenter(sf::Vector2f(e->size) / 2.f);
        Interface::styleManager.onResize(e->size);
        if (Interface::fontManager.load(Interface::styleManager.getScale()))
        {
            if (!ImGui::SFML::UpdateFontTexture()) {
                // Keep current font pointers if texture update failed.
            }
        }
    }
}
