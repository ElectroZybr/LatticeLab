#include "FrameTool.h"

#include <SFML/Window/Keyboard.hpp>

#include "App/interaction/picking/PickingSystem.h"
#include "GUI/interface/interface.h"

FrameTool::FrameTool(ToolContext& context) noexcept : ITool(context) {}

void FrameTool::onLeftPressed(sf::Vector2i mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    overlay.boxVisible = true;
    overlay.boxStart = mousePos;
    overlay.boxEnd = mousePos;
}

void FrameTool::onLeftReleased(sf::Vector2i mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    const bool cumulative =
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);

    auto& overlay = ctx.pickingSystem->getOverlay();
    if (overlay.boxVisible) {
        ctx.pickingSystem->processRect(overlay.boxStart, mousePos, cumulative);
        Interface::countSelectedAtom = static_cast<int>(ctx.pickingSystem->getSelectedIndices().size());
    }
    overlay.reset();
}

void FrameTool::onFrame(sf::Vector2i mousePos, float deltaTime) {
    (void)deltaTime;

    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    if (overlay.boxVisible) {
        overlay.boxEnd = mousePos;
    }
}

void FrameTool::reset() {
    ToolContext& ctx = context();
    if (ctx.pickingSystem != nullptr) {
        ctx.pickingSystem->getOverlay().boxVisible = false;
    }
}
