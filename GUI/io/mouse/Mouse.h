#pragma once
#include <memory>

#include <SFML/Graphics.hpp>

#include "Engine/physics/AtomData.h"
#include "Engine/SimBox.h"
#include "Rendering/BaseRenderer.h"

class Mouse {
    friend class EventManager;
public:
    static void init(sf::RenderWindow* w, std::unique_ptr<IRenderer>&, SimBox* b, std::vector<AtomData>* a);

    static void onEvent(const sf::Event& event);
    static void onFrame(float deltaTime);

    static void logMousePos();
private:
    static sf::RenderWindow* window;
    static std::unique_ptr<IRenderer>* renderer;
    static SimBox*           box;
    static std::vector<AtomData>* atoms;
};
