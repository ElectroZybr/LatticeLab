#include "RulerTool.h"

#include <cstdio>
#include <string>

#include "App/interaction/picking/PickingSystem.h"
#include "Engine/Consts.h"
#include "GUI/interface/UiState.h"
#include "Rendering/BaseRenderer.h"

namespace {

    Vec3f mapMouseToRulerWorld(const ToolContext& ctx, sf::Vector2i mousePos) {
        IRenderer* renderer = ctx.activeRenderer();
        if (renderer == nullptr) {
            return Vec3f();
        }

        if (renderer->camera.getMode() == Camera::Mode::Mode2D && ctx.window != nullptr && ctx.gameView != nullptr) {
            const sf::Vector2f world = ctx.window->mapPixelToCoords(mousePos, *ctx.gameView);
            return Vec3f(world.x, world.y, 1.0f);
        }

        return renderer->camera.screenToWorld(mousePos);
    }

    std::string makeRulerTooltip(const Vec3f& start, const Vec3f& end) {
        const float distanceAngstrom = (end - start).abs();
        const float distanceNm = distanceAngstrom * static_cast<float>(Units::AngstromToNm);

        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "Distance: %.2f A (%.2f nm)", distanceAngstrom, distanceNm);
        return buffer;
    }
}

RulerTool::RulerTool(ToolContext& context) noexcept : ITool(context) {}

void RulerTool::onLeftPressed(sf::Vector2i mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    overlay.rulerVisible = true;
    startWorld_ = mapMouseToRulerWorld(ctx, mousePos);
    endWorld_ = startWorld_;
    hasMeasurement_ = true;
    dragging_ = true;
    syncOverlayFromWorld();

    if (ctx.uiState != nullptr) {
        ctx.uiState->drawToolTrip = false;
        ctx.uiState->toolTooltipText.clear();
    }
    overlay.rulerLabel = makeRulerTooltip(startWorld_, endWorld_);
}

void RulerTool::onLeftReleased(sf::Vector2i mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    endWorld_ = mapMouseToRulerWorld(ctx, mousePos);
    dragging_ = false;
    syncOverlayFromWorld();
    if (ctx.uiState != nullptr) {
        ctx.uiState->drawToolTrip = false;
        ctx.uiState->toolTooltipText.clear();
    }
}

bool RulerTool::onRightPressed(sf::Vector2i mousePos) {
    (void)mousePos;
    clearMeasurement();
    return true;
}

void RulerTool::onFrame(sf::Vector2i mousePos, float deltaTime) {
    (void)deltaTime;
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    if (!overlay.rulerVisible || !hasMeasurement_) {
        return;
    }

    if (dragging_) {
        updateMeasurement(mousePos);
    }
    else {
        syncOverlayFromWorld();
    }
}

void RulerTool::reset() {
    dragging_ = false;
    hasMeasurement_ = false;
    ToolContext& ctx = context();
    if (ctx.uiState != nullptr) {
        ctx.uiState->drawToolTrip = false;
        ctx.uiState->toolTooltipText.clear();
    }
}

void RulerTool::clearMeasurement() {
    ToolContext& ctx = context();
    if (ctx.pickingSystem != nullptr) {
        auto& overlay = ctx.pickingSystem->getOverlay();
        overlay.rulerVisible = false;
        overlay.rulerLabel.clear();
    }
    hasMeasurement_ = false;
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

    endWorld_ = mapMouseToRulerWorld(ctx, mousePos);
    syncOverlayFromWorld();
    if (ctx.uiState != nullptr) {
        ctx.uiState->drawToolTrip = false;
        ctx.uiState->toolTooltipText.clear();
    }
    overlay.rulerLabel = makeRulerTooltip(startWorld_, endWorld_);
}

void RulerTool::syncOverlayFromWorld() {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr || !hasMeasurement_) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    IRenderer* renderer = ctx.activeRenderer();
    if (renderer == nullptr) {
        return;
    }

    if (renderer->camera.getMode() == Camera::Mode::Mode2D && ctx.window != nullptr && ctx.gameView != nullptr) {
        overlay.rulerStart = ctx.window->mapCoordsToPixel(sf::Vector2f(startWorld_.x, startWorld_.y), *ctx.gameView);
        overlay.rulerEnd = ctx.window->mapCoordsToPixel(sf::Vector2f(endWorld_.x, endWorld_.y), *ctx.gameView);
        return;
    }

    overlay.rulerStart = renderer->camera.worldToScreen(startWorld_);
    overlay.rulerEnd = renderer->camera.worldToScreen(endWorld_);
}
