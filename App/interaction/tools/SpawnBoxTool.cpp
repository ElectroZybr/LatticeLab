#include "SpawnBoxTool.h"

#include <algorithm>

#include "GUI/interface/UiState.h"
#include "GUI/interface/panels/io/ioPanel.h"
#include "App/interaction/picking/PickingSystem.h"
#include "Lattice/Engine/Simulation.h"

SpawnBoxTool::SpawnBoxTool(ToolContext& context) noexcept : ITool(context) {}

void SpawnBoxTool::onLeftPressed(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    startLocalWorld_ = screenToLocalWorld(mousePos);
    auto& overlay = ctx.pickingSystem->getOverlay();
    overlay.boxVisible = true;
    overlay.boxStart = mousePos;
    overlay.boxEnd = mousePos;
}

void SpawnBoxTool::onLeftReleased(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr || ctx.simulation == nullptr || ctx.ioPanel == nullptr) {
        reset();
        return;
    }

    const glm::vec3 endLocalWorld = screenToLocalWorld(mousePos);
    const glm::vec3 worldSize = ctx.simulation->world().getWorldSize();
    AppSignals::UI::GeneratorRegionSpec region;
    region.kind = AppSignals::UI::GeneratorRegionKind::Box;
    region.center = glm::vec3(
        0.5f * (startLocalWorld_.x + endLocalWorld.x),
        0.5f * (startLocalWorld_.y + endLocalWorld.y),
        0.5f * worldSize.z
    );
    region.boxSize = glm::vec3(
        std::abs(endLocalWorld.x - startLocalWorld_.x),
        std::abs(endLocalWorld.y - startLocalWorld_.y),
        worldSize.z
    );

    if (region.boxSize.x > 1e-3f && region.boxSize.y > 1e-3f) {
        if (ctx.uiState != nullptr) {
            ctx.ioPanel->emitSpawnFromRegion(region, ctx.uiState->spawnSpecies);
        }
        else {
            ctx.ioPanel->emitSpawnFromRegion(region);
        }
    }

    reset();
}

void SpawnBoxTool::onFrame(glm::ivec2 mousePos, float) {
    ToolContext& ctx = context();
    if (ctx.pickingSystem == nullptr) {
        return;
    }

    auto& overlay = ctx.pickingSystem->getOverlay();
    if (overlay.boxVisible) {
        overlay.boxEnd = mousePos;
    }
}

void SpawnBoxTool::reset() {
    ToolContext& ctx = context();
    if (ctx.pickingSystem != nullptr) {
        auto& overlay = ctx.pickingSystem->getOverlay();
        overlay.boxVisible = false;
    }
    if (ctx.uiState != nullptr) {
        ctx.uiState->drawToolTrip = false;
        ctx.uiState->toolTooltipText.clear();
    }
}
