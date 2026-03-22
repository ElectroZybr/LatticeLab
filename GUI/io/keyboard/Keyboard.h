#pragma once
#include <memory>

#include <SFML/Graphics.hpp>

#include "Rendering/BaseRenderer.h"
#include "GUI/io/keyboard/KeyboardCommand.h"

class Keyboard {
    friend class EventManager;
public:
    static void init(std::unique_ptr<IRenderer>& r);

    static void onEvent(const sf::Event& event);
    static void onFrame(float deltaTime);

    static std::optional<KeyboardCommand> popResult();
private:
    static std::unique_ptr<IRenderer>* render;

    static std::optional<KeyboardCommand> pendingResult;
};
