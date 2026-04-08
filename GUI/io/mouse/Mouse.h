#pragma once
#include <memory>

#include <SFML/Graphics.hpp>

#include "Rendering/BaseRenderer.h"

class Mouse {
    friend class EventManager;

public:
    static void init(sf::RenderWindow& w, std::unique_ptr<IRenderer>& renderer, class Simulation& simulation, class Interface& ui);

    static void onEvent(const sf::Event& event);
    static void onFrame(float deltaTime);

    static void logMousePos();

private:
    static sf::RenderWindow* window;
    static std::unique_ptr<IRenderer>* renderer;
    static class Interface* ui;
};
