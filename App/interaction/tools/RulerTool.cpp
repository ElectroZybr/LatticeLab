#include "RulerTool.h"

#include <cstdio>
#include <string>

#include "App/interaction/picking/PickingSystem.h"
#include "GUI/interface/interface.h"

namespace {
constexpr float kAngstromToNm = 0.1f;

std::string makeRulerTooltip(const Vec3f& start, const Vec3f& end) {
    const float distanceAngstrom = (end - start).abs();
    const float distanceNm = distanceAngstrom * kAngstromToNm;

    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "Distance: %.2f A | %.2f nm", distanceAngstrom, distanceNm);
    return buffer;
}
}

RulerTool::RulerTool(ToolContext& context) noexcept
    : ITool(context) {}

void RulerTool::onLeftPressed(sf::Vector2i mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    overlay.rulerVisible = true;
    overlay.rulerStart = mousePos;
    overlay.rulerEnd = mousePos;
    dragging_ = true;

    Interface::drawToolTrip = true;
    Interface::toolTooltipText = makeRulerTooltip(screenToWorld(mousePos), screenToWorld(mousePos));
}

void RulerTool::onLeftReleased(sf::Vector2i mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    ctx.pickingSystem->getOverlay().rulerEnd = mousePos;
    dragging_ = false;
    Interface::drawToolTrip = false;
}

bool RulerTool::onRightPressed(sf::Vector2i mousePos) {
    (void)mousePos;
    clearMeasurement();
    return true;
}

void RulerTool::onFrame(sf::Vector2i mousePos, float deltaTime) {
    (void)deltaTime;
    if (!dragging_) {
        return;
    }
    updateMeasurement(mousePos);
}

void RulerTool::reset() {
    dragging_ = false;
    Interface::drawToolTrip = false;
    Interface::toolTooltipText.clear();
}

void RulerTool::clearMeasurement() {
    ToolContext& ctx = context();
    if (ctx.pickingSystem != nullptr) {
        ctx.pickingSystem->getOverlay().rulerVisible = false;
    }
    reset();
}

void RulerTool::updateMeasurement(sf::Vector2i mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    if (!overlay.rulerVisible) {
        return;
    }

    overlay.rulerEnd = mousePos;
    Interface::drawToolTrip = true;
    Interface::toolTooltipText = makeRulerTooltip(screenToWorld(overlay.rulerStart), screenToWorld(mousePos));
}
