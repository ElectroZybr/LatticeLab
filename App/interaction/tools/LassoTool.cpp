#include "LassoTool.h"

#include <SFML/Window/Keyboard.hpp>

#include "App/interaction/picking/PickingSystem.h"
#include "GUI/interface/interface.h"

LassoTool::LassoTool(ToolContext& context) noexcept
    : ITool(context) {}

void LassoTool::onLeftPressed(sf::Vector2i mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    overlay.lassoVisible = true;
    overlay.lassoPoints.clear();
    overlay.lassoPoints.push_back(mousePos);
}

void LassoTool::onLeftReleased(sf::Vector2i mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    const bool cumulative = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);

    auto& overlay = ctx.pickingSystem->getOverlay();
    if (overlay.lassoVisible) {
        if (overlay.lassoPoints.empty() || overlay.lassoPoints.back() != mousePos) {
            overlay.lassoPoints.push_back(mousePos);
        }
        ctx.pickingSystem->processLasso(overlay.lassoPoints, cumulative);
        Interface::countSelectedAtom = static_cast<int>(ctx.pickingSystem->getSelectedIndices().size());
    }
    overlay.reset();
}

void LassoTool::onFrame(sf::Vector2i mousePos, float deltaTime) {
    (void)deltaTime;

    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    if (!overlay.lassoVisible) {
        return;
    }

    constexpr float kMinStepSqr = 25.0f;
    if (overlay.lassoPoints.empty()) {
        overlay.lassoPoints.push_back(mousePos);
        return;
    }

    const sf::Vector2f currentPos(mousePos.x, mousePos.y);
    const sf::Vector2f lastPos(overlay.lassoPoints.back().x, overlay.lassoPoints.back().y);
    if ((currentPos - lastPos).lengthSquared() >= kMinStepSqr) {
        overlay.lassoPoints.push_back(mousePos);
    }
}

void LassoTool::reset() {
    ToolContext& ctx = context();
    if (ctx.pickingSystem != nullptr) {
        auto& overlay = ctx.pickingSystem->getOverlay();
        overlay.lassoVisible = false;
        overlay.lassoPoints.clear();
    }
}
