#include "AreaTool.h"

#include <cmath>

#include "App/interaction/picking/PickingSystem.h"
#include "GUI/interface/UiState.h"
#include "GUI/interface/panels/io/ioPanel.h"
#include "GUI/interface/panels/tools/SideToolsPanel.h"
#include "GUI/io/keyboard/Keyboard.h"
#include "Lattice/Engine/Simulation.h"

namespace {
    constexpr float kMinLassoStepSqr = 25.0f;
    constexpr float kMinBrushStepSqr = 16.0f;
}

AreaTool::AreaTool(ToolContext& context, SideToolsPanel& sideToolsPanel) noexcept : ITool(context), sideToolsPanel_(sideToolsPanel) {}

void AreaTool::onLeftPressed(glm::ivec2 mousePos) {
    switch (sideToolsPanel_.areaShape()) {
    case SideToolsPanel::AreaShape::Rect:
        beginRect(mousePos);
        break;
    case SideToolsPanel::AreaShape::Lasso:
        beginLasso(mousePos);
        break;
    case SideToolsPanel::AreaShape::Brush:
        beginBrush(mousePos);
        break;
    }
}

void AreaTool::onLeftReleased(glm::ivec2 mousePos) {
    switch (sideToolsPanel_.areaShape()) {
    case SideToolsPanel::AreaShape::Rect:
        finishRect(mousePos);
        break;
    case SideToolsPanel::AreaShape::Lasso:
        finishLasso(mousePos);
        break;
    case SideToolsPanel::AreaShape::Brush:
        finishBrush(mousePos);
        break;
    }
}

void AreaTool::onFrame(glm::ivec2 mousePos, float) {
    const bool uiHovered = context().uiState != nullptr && context().uiState->cursorHovered;
    switch (sideToolsPanel_.areaShape()) {
    case SideToolsPanel::AreaShape::Rect:
        if (!uiHovered) {
            updateRect(mousePos);
        }
        break;
    case SideToolsPanel::AreaShape::Lasso:
        if (!uiHovered) {
            updateLasso(mousePos);
        }
        break;
    case SideToolsPanel::AreaShape::Brush:
        updateBrush(mousePos, uiHovered);
        break;
    }
}

void AreaTool::reset() {
    ToolContext& ctx = context();
    brushActive_ = false;
    brushSelectionActive_ = false;
    brushSelectionCumulative_ = false;
    lastBrushSample_ = glm::ivec2(std::numeric_limits<int>::min());
    if (ctx.pickingSystem != nullptr) {
        ctx.pickingSystem->getOverlay().reset();
    }
}

void AreaTool::beginRect(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    rectStartLocalWorld_ = screenToLocalWorld(mousePos);
    auto& overlay = ctx.pickingSystem->getOverlay();
    overlay.boxVisible = true;
    overlay.boxStart = mousePos;
    overlay.boxEnd = mousePos;
}

void AreaTool::beginLasso(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    overlay.lassoVisible = true;
    overlay.lassoPoints.clear();
    overlay.lassoPoints.emplace_back(mousePos);
}

void AreaTool::beginBrush(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    overlay.circleVisible = true;
    overlay.circleCenter = mousePos;
    overlay.circleRadius = brushRadiusScreen(mousePos);
    brushActive_ = true;
    brushSelectionActive_ = sideToolsPanel_.areaAction() == SideToolsPanel::AreaAction::Select;
    brushSelectionCumulative_ = cumulativeSelection();
    lastBrushSample_ = glm::ivec2(std::numeric_limits<int>::min());
    applyBrushAt(mousePos);
}

void AreaTool::finishRect(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    if (sideToolsPanel_.areaAction() == SideToolsPanel::AreaAction::Select) {
        ctx.pickingSystem->processRect(ctx.pickingSystem->getOverlay().boxStart, mousePos, cumulativeSelection());
        updateSelectionCount();
    }
    else if (ctx.ioPanel != nullptr) {
        ctx.ioPanel->emitSpawnFromRegion(makeRectRegion(mousePos),
                                         ctx.uiState != nullptr ? std::string_view(ctx.uiState->spawnSpecies) : std::string_view{});
    }

    ctx.pickingSystem->getOverlay().reset();
}

void AreaTool::finishLasso(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    if (overlay.lassoPoints.empty() || overlay.lassoPoints.back() != mousePos) {
        overlay.lassoPoints.emplace_back(mousePos);
    }

    if (sideToolsPanel_.areaAction() == SideToolsPanel::AreaAction::Select) {
        ctx.pickingSystem->processLasso(overlay.lassoPoints, cumulativeSelection());
        updateSelectionCount();
    }
    else if (ctx.ioPanel != nullptr) {
        ctx.ioPanel->emitSpawnFromRegion(makeLassoRegion(),
                                         ctx.uiState != nullptr ? std::string_view(ctx.uiState->spawnSpecies) : std::string_view{});
    }

    overlay.reset();
}

void AreaTool::finishBrush(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    applyBrushAt(mousePos);
    brushActive_ = false;
    brushSelectionActive_ = false;
    brushSelectionCumulative_ = false;
    lastBrushSample_ = glm::ivec2(std::numeric_limits<int>::min());
    auto& overlay = ctx.pickingSystem->getOverlay();
    overlay.circleVisible = false;
    overlay.circleRadius = 0.0f;
}

void AreaTool::updateRect(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }
    auto& overlay = ctx.pickingSystem->getOverlay();
    if (overlay.boxVisible) {
        overlay.boxEnd = mousePos;
    }
}

void AreaTool::updateLasso(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    if (!overlay.lassoVisible) {
        return;
    }

    if (overlay.lassoPoints.empty()) {
        overlay.lassoPoints.emplace_back(mousePos);
        return;
    }

    const glm::vec2 currentPos(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
    const glm::ivec2& last = overlay.lassoPoints.back();
    const glm::vec2 lastPos(static_cast<float>(last.x), static_cast<float>(last.y));
    const glm::vec2 diff = currentPos - lastPos;
    if ((diff.x * diff.x + diff.y * diff.y) >= kMinLassoStepSqr) {
        overlay.lassoPoints.emplace_back(mousePos);
    }
}

void AreaTool::updateBrush(glm::ivec2 mousePos, bool uiHovered) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    overlay.circleVisible = true;
    overlay.circleCenter = mousePos;
    overlay.circleRadius = brushRadiusScreen(mousePos);
    if (brushActive_ && !uiHovered) {
        applyBrushAt(mousePos);
    }
}

void AreaTool::applyBrushAt(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    if (lastBrushSample_.x != std::numeric_limits<int>::min()) {
        const glm::vec2 diff = glm::vec2(mousePos - lastBrushSample_);
        if ((diff.x * diff.x + diff.y * diff.y) < kMinBrushStepSqr) {
            return;
        }
    }
    lastBrushSample_ = mousePos;

    if (sideToolsPanel_.areaAction() == SideToolsPanel::AreaAction::Select) {
        ctx.pickingSystem->processCircle(mousePos, brushRadiusScreen(mousePos), brushSelectionCumulative_);
        brushSelectionCumulative_ = true;
        updateSelectionCount();
        return;
    }

    if (ctx.ioPanel != nullptr) {
        ctx.ioPanel->emitSpawnFromRegion(makeBrushRegion(mousePos),
                                         ctx.uiState != nullptr ? std::string_view(ctx.uiState->spawnSpecies) : std::string_view{});
    }
}

float AreaTool::brushRadiusScreen(glm::ivec2 mousePos) const {
    const glm::vec3 localCenter = screenToLocalWorld(mousePos);
    const glm::vec3 renderOffset = context().simulation != nullptr ? context().simulation->world().getRenderOffset() : glm::vec3(0.0f);
    const glm::ivec2 centerScreen = worldToScreen(localCenter + renderOffset);
    const glm::ivec2 edgeScreen = worldToScreen(localCenter + renderOffset + glm::vec3(sideToolsPanel_.brushRadius(), 0.0f, 0.0f));
    const glm::vec2 delta = glm::vec2(edgeScreen - centerScreen);
    const float radius = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    return std::max(radius, 2.0f);
}

AppSignals::UI::GeneratorRegionSpec AreaTool::makeRectRegion(glm::ivec2 mousePos) const {
    const ToolContext& ctx = context();
    const glm::vec3 endLocalWorld = screenToLocalWorld(mousePos);
    const glm::vec3 worldSize = ctx.simulation->world().getWorldSize();

    AppSignals::UI::GeneratorRegionSpec region;
    region.kind = AppSignals::UI::GeneratorRegionKind::Box;
    region.center = glm::vec3(
        0.5f * (rectStartLocalWorld_.x + endLocalWorld.x),
        0.5f * (rectStartLocalWorld_.y + endLocalWorld.y),
        0.5f * worldSize.z
    );
    region.boxSize = glm::vec3(
        std::abs(endLocalWorld.x - rectStartLocalWorld_.x),
        std::abs(endLocalWorld.y - rectStartLocalWorld_.y),
        worldSize.z
    );
    return region;
}

AppSignals::UI::GeneratorRegionSpec AreaTool::makeLassoRegion() const {
    const ToolContext& ctx = context();
    AppSignals::UI::GeneratorRegionSpec region;
    region.kind = AppSignals::UI::GeneratorRegionKind::PolygonPrism;
    region.polygonPoints.clear();
    if (ctx.pickingSystem != nullptr) {
        for (const glm::ivec2& point : ctx.pickingSystem->getOverlay().lassoPoints) {
            const glm::vec3 local = screenToLocalWorld(point);
            region.polygonPoints.emplace_back(local.x, local.y);
        }
    }
    region.prismMinZ = 0.0f;
    region.prismMaxZ = ctx.simulation != nullptr ? ctx.simulation->world().getWorldSize().z : 0.0f;
    return region;
}

AppSignals::UI::GeneratorRegionSpec AreaTool::makeBrushRegion(glm::ivec2 mousePos) const {
    const ToolContext& ctx = context();
    const glm::vec3 localCenter = screenToLocalWorld(mousePos);
    const glm::vec3 worldSize = ctx.simulation->world().getWorldSize();

    AppSignals::UI::GeneratorRegionSpec region;
    region.kind = AppSignals::UI::GeneratorRegionKind::Cylinder;
    region.center = glm::vec3(localCenter.x, localCenter.y, 0.5f * worldSize.z);
    region.cylinderRadius = sideToolsPanel_.brushRadius();
    region.cylinderHeight = worldSize.z;
    return region;
}

void AreaTool::updateSelectionCount() const {
    const ToolContext& ctx = context();
    if (ctx.uiState != nullptr && ctx.pickingSystem != nullptr) {
        ctx.uiState->selectedAtomCount = static_cast<int>(ctx.pickingSystem->getSelectedAtomIds().size());
    }
}

bool AreaTool::cumulativeSelection() const {
    return Keyboard::isPressed(GLFW_KEY_LEFT_CONTROL) || Keyboard::isPressed(GLFW_KEY_RIGHT_CONTROL);
}
