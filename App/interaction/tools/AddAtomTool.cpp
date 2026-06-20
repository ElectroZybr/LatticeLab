#include "AddAtomTool.h"

#include <cstdlib>

#include "Lattice/Engine/Simulation.h"
#include "GUI/interface/UiState.h"
#include "GUI/interface/panels/periodic/PeriodicPanel.h"
#include "Rendering/BaseRenderer.h"

namespace {
    glm::vec3 randomVelocity(float scale) {
        auto randomComponent = []() {
            return 2.0f * static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 1.0f;
        };
        return glm::vec3(randomComponent(), randomComponent(), randomComponent()) * scale;
    }
}

AddAtomTool::AddAtomTool(ToolContext& context) noexcept : ITool(context) {}

void AddAtomTool::onLeftPressed(glm::ivec2 mousePos) {
    ToolContext& ctx = context();
    if (!ctx.isValid()) {
        return;
    }

    if (ctx.uiState == nullptr) {
        return;
    }

    const AtomData::Type atomType = static_cast<AtomData::Type>(PeriodicPanel::decodeAtom(ctx.uiState->selectedAtom));
    const bool spawnMolecule = ctx.simulation->findMoleculeTemplate(ctx.uiState->spawnSpecies) != nullptr;
    glm::vec3 spawnPos = screenToLocalWorld(mousePos);
    const std::optional<glm::mat3> moleculeRotation =
        spawnMolecule ? std::optional<glm::mat3>(Lattice::Simulation::randomRotationMatrix()) : std::nullopt;

    if (!ctx.simulation->canSpawnMolecule(ctx.uiState->spawnSpecies, spawnPos, moleculeRotation)) {
        return;
    }

    glm::vec3 velocity = randomVelocity(5.0f);
    if (spawnMolecule) {
        (void)ctx.simulation->spawnMoleculeChecked(ctx.uiState->spawnSpecies, spawnPos, moleculeRotation, false);
        return;
    }

    ctx.simulation->createAtom(spawnPos, velocity, atomType, false);
}
