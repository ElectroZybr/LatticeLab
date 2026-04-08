#pragma once

#include <vector>

#include <SFML/Graphics.hpp>

class IRenderer;
class Simulation;
class Interface;

class EventManager {
public:
    static void init(sf::RenderWindow& window, sf::View& uiView, Simulation& simulation, std::unique_ptr<IRenderer>& renderer, Interface& ui);
    static void poll();
    static void frame(float deltaTime);

private:
    static sf::RenderWindow* window;
    static std::unique_ptr<IRenderer>* renderer;
};
