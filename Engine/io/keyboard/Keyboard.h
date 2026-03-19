#pragma once
#include <SFML/Graphics.hpp>
#include "Engine/renderer/IRenderer.h"

class Keyboard {
    friend class EventManager;
public:
    static void init(IRenderer* r);

    static void onEvent(const sf::Event& event);
    static void onFrame(float deltaTime);

private:
    static IRenderer* render;
};
