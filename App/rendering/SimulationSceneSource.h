#pragma once

#include <unordered_set>

#include "Engine/Simulation.h"
#include "Rendering/BaseRenderer.h"

namespace App::Rendering {
    inline RenderAtomsView makeRenderAtomsView(const World& world) {
        const AtomStorage& atoms = world.getAtomStorage();
        return RenderAtomsView{
            .count = atoms.size(),
            .x = atoms.xData(),
            .y = atoms.yData(),
            .z = atoms.zData(),
            .vx = atoms.vxData(),
            .vy = atoms.vyData(),
            .vz = atoms.vzData(),
            .type = atoms.atomTypeData(),
            .radius = nullptr,
        };
    }

    inline void copySelection(RenderData& renderData, const std::unordered_set<size_t>* selectedIndices) {
        renderData.selectedAtomIndices.clear();
        if (selectedIndices == nullptr) {
            return;
        }

        renderData.selectedAtomIndices.reserve(selectedIndices->size());
        for (const size_t index : *selectedIndices) {
            renderData.selectedAtomIndices.push_back(index);
        }
    }

    inline void syncRendererWithSimulation(BaseRenderer& renderer, const Simulation& simulation,
                                           const std::unordered_set<size_t>* selectedIndices = nullptr) {
        renderer.resizeRenderData(simulation.worldCount());

        for (Simulation::WorldId worldId = 0; worldId < simulation.worldCount(); ++worldId) {
            const World& world = simulation.worldAt(worldId);
            RenderData& renderData = renderer.getRenderData(worldId);

            renderData.atoms = makeRenderAtomsView(world);
            renderData.worldSize = {world.getWorldSize().x, world.getWorldSize().y, world.getWorldSize().z};
            renderData.renderOffset = {world.getRenderOffset().x, world.getRenderOffset().y, world.getRenderOffset().z};
            renderData.bonds = &world.getBonds();
            renderData.grid = &world.getGrid();
            renderData.selectedAtomIndices.clear();
        }

        if (simulation.worldCount() == 0) {
            return;
        }

        const World& activeWorld = simulation.worldAt(simulation.activeWorldId());
        renderer.camera.setSceneBounds(activeWorld.getWorldSize(), activeWorld.getRenderOffset());
        if (selectedIndices != nullptr) {
            copySelection(renderer.getRenderData(simulation.activeWorldId()), selectedIndices);
        }
    }
}
