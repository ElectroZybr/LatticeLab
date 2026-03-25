#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

class IRenderer;
class SimBox;
class AtomStorage;

class EventManager {
public:
    static void init(sf::RenderWindow* w, sf::View* ui, std::unique_ptr<IRenderer>& r, SimBox* b, AtomStorage* atomStorage);
    static void poll();
    static void frame(float deltaTime);

private:
    static sf::RenderWindow* window;
    static std::unique_ptr<IRenderer>* renderer;
};
