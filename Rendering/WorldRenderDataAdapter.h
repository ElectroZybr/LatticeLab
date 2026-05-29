#pragma once

#include "Engine/Simulation.h"
#include "Engine/World.h"
#include "Rendering/BaseRenderer.h"
#include "Rendering/RenderData.h"

namespace Rendering {
    inline void fillRenderDataFromWorld(RenderData& renderData, const World& world) {
        const AtomStorage& atoms = world.getAtomStorage();

        renderData.atoms = RenderAtomsView{
            .count = atoms.size(),
            .x = atoms.xData(),
            .y = atoms.yData(),
            .z = atoms.zData(),
            .vx = atoms.vxData(),
            .vy = atoms.vyData(),
            .vz = atoms.vzData(),
            .type = atoms.atomTypeData(),
        };

        renderData.worldSize = world.getWorldSize();
        renderData.renderOffset = world.getRenderOffset();

        renderData.bonds.clear();
        renderData.bonds.reserve(world.getBonds().size());
        for (const Bond& bond : world.getBonds()) {
            renderData.bonds.push_back(RenderBond{
                .aIndex = bond.aIndex,
                .bIndex = bond.bIndex,
            });
        }

        renderData.gridCells.clear();
        const SpatialGrid& grid = world.getGrid();
        for (unsigned int z = 1; z < grid.size.z - 1; ++z) {
            for (unsigned int y = 1; y < grid.size.y - 1; ++y) {
                for (unsigned int x = 1; x < grid.size.x - 1; ++x) {
                    const int atomCount = grid.countAtomsInCell(x, y, z);
                    if (atomCount > 0) {
                        renderData.gridCells.push_back(RenderGridCell{
                            .origin = Vec3f((x - 1) * grid.cellSize, (y - 1) * grid.cellSize, (z - 1) * grid.cellSize),
                            .cellSize = static_cast<float>(grid.cellSize),
                            .atomCount = static_cast<float>(atomCount),
                        });
                    }
                }
            }
        }
    }

    inline void syncRendererWithSimulation(BaseRenderer& renderer, const Simulation& simulation) {
        while (renderer.getRenderDataCount() < simulation.worldCount()) {
            renderer.addRenderData();
        }

        for (Simulation::WorldId worldId = 0; worldId < simulation.worldCount(); ++worldId) {
            RenderData& renderData = renderer.getRenderData(worldId);
            fillRenderDataFromWorld(renderData, simulation.worldAt(worldId));
        }

        if (simulation.worldCount() > 0) {
            const World& activeWorld = simulation.worldAt(simulation.activeWorldId());
            renderer.camera.setSceneBounds(activeWorld.getWorldSize(), activeWorld.getRenderOffset());
        }
    }
}