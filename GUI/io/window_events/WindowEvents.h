#pragma once
#include <SFML/Graphics.hpp>

class WindowEvents {
    friend class EventManager;

public:
    static void init(sf::RenderWindow& w, sf::View& uiView, class Interface& ui);
    static void onEvent(const sf::Event& event);

private:
    static sf::RenderWindow* window;
    static sf::View* gameView;
    static class Interface* ui;
};
