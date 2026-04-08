#pragma once
#include <memory>

#include <SFML/Graphics.hpp>

#include "Rendering/BaseRenderer.h"

class Keyboard {
    friend class EventManager;

public:
    static void init(std::unique_ptr<IRenderer>& r, class Interface& appInterface);

    static void onEvent(const sf::Event& event);
    static void onFrame(float deltaTime);

private:
    static std::unique_ptr<IRenderer>* render;
    static class Interface* appInterface;
};
