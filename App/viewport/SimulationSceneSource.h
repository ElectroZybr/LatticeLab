#pragma once

#include <unordered_set>

#include "Lattice/Engine/Simulation.h"
#include "Lattice/Engine/physics/VectorField.h"
#include "Rendering/BaseRenderer.h"

namespace App::Viewport {
    inline glm::vec3 makeRenderBoxSize(const World& world) {
        return world.getWorldSize();
    }

    inline void forEachWorldBond(const void* context, RenderBondVisitor visitor, void* userData) {
        const auto& bonds = *static_cast<const Bond::List*>(context);
        for (const Bond& bond : bonds) {
            visitor(bond.aIndex, bond.bIndex, userData);
        }
    }

    inline void forEachWorldGridCell(const void* context, RenderGridCellVisitor visitor, void* userData) {
        const auto& grid = *static_cast<const SpatialGrid*>(context);
        for (unsigned int z = 1; z < grid.size.z - 1; ++z) {
            for (unsigned int y = 1; y < grid.size.y - 1; ++y) {
                for (unsigned int x = 1; x < grid.size.x - 1; ++x) {
                    const int atomCount = grid.countAtomsInCell(x, y, z);
                    if (atomCount <= 0) {
                        continue;
                    }

                    const RenderGridCell cell{
                        .origin = glm::vec3(static_cast<float>((x - 1) * grid.cellSize), static_cast<float>((y - 1) * grid.cellSize),
                                            static_cast<float>((z - 1) * grid.cellSize)),
                        .cellSize = static_cast<float>(grid.cellSize),
                        .atomCount = static_cast<float>(atomCount),
                    };
                    visitor(cell, userData);
                }
            }
        }
    }

    inline void forEachVectorFieldCell(const void* context, RenderVectorFieldCellVisitor visitor, void* userData) {
        const auto& field = *static_cast<const VectorField*>(context);
        const glm::ivec3 size = field.gridSize();
        const float cellScale = field.cellScale();
        const float z = static_cast<float>(field.zSlice()) * cellScale;

        for (int y = 0; y < size.y; ++y) {
            for (int x = 0; x < size.x; ++x) {
                const float potential = field.potentialAt(x, y);
                if (potential == 0.0f) {
                    continue;
                }

                const RenderGridCell cell{
                    .origin = glm::vec3(static_cast<float>(x) * cellScale, static_cast<float>(y) * cellScale, z),
                    .cellSize = cellScale,
                    .atomCount = potential,
                };
                visitor(cell, userData);
            }
        }
    }

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
            .type = reinterpret_cast<const uint8_t*>(atoms.atomTypeData()),
            .radius = nullptr,
        };
    }

    inline void copySelection(RenderData& renderData, const AtomStorage& atoms, const std::unordered_set<AtomStorage::AtomId>* selectedAtomIds) {
        renderData.selectedAtomIndices.clear();
        if (selectedAtomIds == nullptr) {
            return;
        }

        renderData.selectedAtomIndices.reserve(selectedAtomIds->size());
        for (const AtomStorage::AtomId atomId : *selectedAtomIds) {
            const size_t index = atoms.indexOf(atomId);
            if (index < atoms.size()) {
                renderData.selectedAtomIndices.push_back(index);
            }
        }
    }

    inline void syncRendererWithSimulation(BaseRenderer& renderer, const Lattice::Simulation& simulation,
                                           const std::unordered_set<AtomStorage::AtomId>* selectedAtomIds = nullptr) {
        renderer.resizeRenderData(simulation.worldCount());

        for (Lattice::Simulation::WorldId worldId = 0; worldId < simulation.worldCount(); ++worldId) {
            const World& world = simulation.worldAt(worldId);
            RenderData& renderData = renderer.getRenderData(worldId);

            renderData.atoms = makeRenderAtomsView(world);
            renderData.hasBox = true;
            renderData.worldSize = makeRenderBoxSize(world);
            renderData.renderOffset = world.getRenderOffset();
            renderData.isActiveWorld = (worldId == simulation.activeWorldId());
            renderData.bonds = RenderBondsView{
                .context = &world.getBonds(),
                .count = world.getBonds().size(),
                .forEachFn = forEachWorldBond,
            };
            renderData.grid = RenderGridView{
                .context = &world.getGrid(),
                .count = world.getGrid().countCells,
                .forEachFn = forEachWorldGridCell,
            };
            renderData.vectorField = {};
            renderData.drawVectorField = false;
            if (worldId == simulation.activeWorldId()) {
                const VectorField& vectorField = world.getVectorField();
                const glm::ivec3 fieldSize = vectorField.gridSize();
                renderData.vectorField = RenderVectorFieldView{
                    .context = &vectorField,
                    .count = static_cast<size_t>(fieldSize.x) * static_cast<size_t>(fieldSize.y),
                    .forEachFn = forEachVectorFieldCell,
                };
                renderData.drawVectorField = true;
            }
            renderData.selectedAtomIndices.clear();
        }

        if (simulation.worldCount() == 0) {
            return;
        }

        const World& activeWorld = simulation.worldAt(simulation.activeWorldId());
        renderer.camera.setSceneBounds(makeRenderBoxSize(activeWorld), activeWorld.getRenderOffset());
        if (selectedAtomIds != nullptr) {
            copySelection(renderer.getRenderData(simulation.activeWorldId()), activeWorld.getAtomStorage(), selectedAtomIds);
        }
    }
}
