#pragma once
#include <SFML/Graphics.hpp>

#include "Rendering/BaseRenderer.h"
#include "GUI/io/keyboard/KeyboardCommand.h"

class Keyboard {
    friend class EventManager;
public:
    static void init(IRenderer* r);

    static void onEvent(const sf::Event& event);
    static void onFrame(float deltaTime);

    static std::optional<KeyboardCommand> popResult();
private:
    static IRenderer* render;

    static std::optional<KeyboardCommand> pendingResult;
};
