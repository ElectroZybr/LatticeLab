#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

class IRenderer;
class SimBox;
class Atom;

class EventManager {
public:
    static void init(sf::RenderWindow* w, sf::View* ui, IRenderer* r, SimBox* b, std::vector<Atom>* a);
    static void updateRenderer(IRenderer* r);
    static void poll();
    static void frame(float deltaTime);

private:
    static sf::RenderWindow* window;
    static IRenderer*        render;
};