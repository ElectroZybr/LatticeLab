#pragma once
#include <memory>

#include <SFML/Graphics.hpp>

#include "Engine/physics/Atom.h"
#include "Engine/SimBox.h"
#include "Rendering/BaseRenderer.h"

class Mouse {
    friend class EventManager;
public:
    static void init(sf::RenderWindow* w, std::unique_ptr<IRenderer>&, SimBox* b, std::vector<Atom>* a);

    static void onEvent(const sf::Event& event);
    static void onFrame();

    static void logMousePos();
private:
    static sf::RenderWindow* window;
    static std::unique_ptr<IRenderer>* renderer;
    static SimBox*           box;
    static std::vector<Atom>* atoms;
};
