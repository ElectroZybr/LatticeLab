#pragma once

#include <vector>

#include <SFML/Graphics.hpp>

class IRenderer;
class SimBox;
class AtomStorage;
class Interface;

class EventManager {
public:
    static void init(sf::RenderWindow* w, sf::View* uiView, std::unique_ptr<IRenderer>& r, SimBox* b, AtomStorage* atomStorage, Interface* ui);
    static void poll();
    static void frame(float deltaTime);

private:
    static sf::RenderWindow* window;
    static std::unique_ptr<IRenderer>* renderer;
};
