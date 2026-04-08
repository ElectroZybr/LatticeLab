#include "WindowEvents.h"

#include <imgui-SFML.h>

#include "GUI/interface/interface.h"

sf::RenderWindow* WindowEvents::window = nullptr;
sf::View* WindowEvents::gameView = nullptr;
Interface* WindowEvents::ui = nullptr;

void WindowEvents::init(sf::RenderWindow& w, sf::View& gv, Interface& ui) {
    window = &w;
    gameView = &gv;
    WindowEvents::ui = &ui;
}

void WindowEvents::onEvent(const sf::Event& event) {
    if (event.is<sf::Event::Closed>()) {
        window->close();
    }

    if (const auto* e = event.getIf<sf::Event::Resized>()) {
        gameView->setSize(sf::Vector2f(e->size));
        gameView->setCenter(sf::Vector2f(e->size) / 2.f);
        if (ui == nullptr) {
            return;
        }

        ui->styleManager.onResize(e->size);
        if (ui->fontManager.load(ui->styleManager.getScale())) {
            if (!ImGui::SFML::UpdateFontTexture()) {
                // Keep current font pointers if texture update failed.
            }
        }
    }
}
