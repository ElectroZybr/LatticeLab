#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

class IRenderer;
class SimBox;
class AtomData;

class EventManager {
public:
    static void init(sf::RenderWindow* w, sf::View* ui, std::unique_ptr<IRenderer>& r, SimBox* b, std::vector<AtomData>* a);
    static void poll();
    static void frame(float deltaTime);

private:
    static sf::RenderWindow* window;
    static std::unique_ptr<IRenderer>* renderer;
};
