#include "SpawnCircleTool.h"

#include <cmath>

#include "GUI/interface/UiState.h"
#include "GUI/interface/panels/io/ioPanel.h"
#include "App/interaction/picking/PickingSystem.h"
#include "Lattice/Engine/Simulation.h"

SpawnCircleTool::SpawnCircleTool(ToolContext& context) noexcept : ITool(context) {}

void SpawnCircleTool::onLeftPressed(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    centerLocalWorld_ = screenToLocalWorld(mousePos);
    auto& overlay = ctx.pickingSystem->getOverlay();
    overlay.circleVisible = true;
    overlay.circleCenter = mousePos;
    overlay.circleRadius = 0.0f;
}

void SpawnCircleTool::onLeftReleased(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr || ctx.simulation == nullptr || ctx.ioPanel == nullptr) {
        reset();
        return;
    }

    const glm::vec3 edgeLocalWorld = screenToLocalWorld(mousePos);
    const glm::vec2 delta(edgeLocalWorld.x - centerLocalWorld_.x, edgeLocalWorld.y - centerLocalWorld_.y);
    const float radius = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (radius > 1e-3f) {
        const glm::vec3 worldSize = ctx.simulation->world().getWorldSize();
        AppSignals::UI::GeneratorRegionSpec region;
        region.kind = AppSignals::UI::GeneratorRegionKind::Cylinder;
        region.center = glm::vec3(centerLocalWorld_.x, centerLocalWorld_.y, 0.5f * worldSize.z);
        region.cylinderRadius = radius;
        region.cylinderHeight = worldSize.z;
        if (ctx.uiState != nullptr) {
            ctx.ioPanel->emitSpawnFromRegion(region, ctx.uiState->spawnSpecies);
        }
        else {
            ctx.ioPanel->emitSpawnFromRegion(region);
        }
    }

    reset();
}

void SpawnCircleTool::onFrame(glm::ivec2 mousePos, float) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    if (overlay.circleVisible) {
        const glm::vec2 delta = glm::vec2(mousePos - overlay.circleCenter);
        overlay.circleRadius = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    }
}

void SpawnCircleTool::reset() {
    ToolContext& ctx = context();
    if (ctx.pickingSystem != nullptr) {
        auto& overlay = ctx.pickingSystem->getOverlay();
        overlay.circleVisible = false;
        overlay.circleRadius = 0.0f;
    }
    if (ctx.uiState != nullptr) {
        ctx.uiState->drawToolTrip = false;
        ctx.uiState->toolTooltipText.clear();
    }
}
