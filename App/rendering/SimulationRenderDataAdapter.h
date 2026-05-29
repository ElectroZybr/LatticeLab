#pragma once

#include <algorithm>
#include <cstdint>

#include "App/rendering/RenderMathAdapters.h"
#include "Engine/Simulation.h"
#include "Engine/World.h"
#include "Rendering/BaseRenderer.h"
#include "Rendering/RenderData.h"

namespace App::Rendering {
    inline void fillAtomPalette(RenderData& renderData) {
        const size_t typeCount = static_cast<size_t>(AtomData::Type::COUNT);
        renderData.typeColors.resize(typeCount);

        for (size_t i = 0; i < typeCount; ++i) {
            const auto& props = AtomData::getProps(static_cast<AtomData::Type>(i));
            renderData.typeColors[i] = RenderColor{
                .r = props.color.r / 255.0f,
                .g = props.color.g / 255.0f,
                .b = props.color.b / 255.0f,
                .a = props.color.a / 255.0f,
            };
        }
    }

    inline void fillAtomRenderViews(RenderData& renderData, const AtomStorage& atoms) {
        const size_t atomCount = atoms.size();

        renderData.ownedTypeIds.resize(atomCount);
        renderData.ownedRadii.resize(atomCount);

        for (size_t i = 0; i < atomCount; ++i) {
            const AtomData::Type atomType = atoms.type(i);
            renderData.ownedTypeIds[i] = static_cast<uint16_t>(atomType);
            renderData.ownedRadii[i] = AtomData::getProps(atomType).radius;
        }

        renderData.atoms = RenderAtomsView{
            .count = atomCount,
            .x = atoms.xData(),
            .y = atoms.yData(),
            .z = atoms.zData(),
            .vx = atoms.vxData(),
            .vy = atoms.vyData(),
            .vz = atoms.vzData(),
            .typeId = renderData.ownedTypeIds.data(),
            .radius = renderData.ownedRadii.data(),
        };
    }

    inline void fillBondRenderData(RenderData& renderData, const World& world) {
        renderData.bonds.clear();
        renderData.bonds.reserve(world.getBonds().size());

        for (const Bond& bond : world.getBonds()) {
            renderData.bonds.push_back(RenderBond{
                .aIndex = bond.aIndex,
                .bIndex = bond.bIndex,
            });
        }
    }

    inline void fillGridRenderData(RenderData& renderData, const World& world) {
        renderData.gridCells.clear();

        const SpatialGrid& grid = world.getGrid();
        for (unsigned int z = 1; z < grid.size.z - 1; ++z) {
            for (unsigned int y = 1; y < grid.size.y - 1; ++y) {
                for (unsigned int x = 1; x < grid.size.x - 1; ++x) {
                    const int atomCount = grid.countAtomsInCell(x, y, z);
                    if (atomCount <= 0) {
                        continue;
                    }

                    renderData.gridCells.push_back(RenderGridCell{
                        .origin = glm::vec3(static_cast<float>((x - 1) * grid.cellSize), static_cast<float>((y - 1) * grid.cellSize),
                                             static_cast<float>((z - 1) * grid.cellSize)),
                        .cellSize = static_cast<float>(grid.cellSize),
                        .atomCount = static_cast<float>(atomCount),
                    });
                }
            }
        }
    }

    inline void fillRenderDataFromWorld(RenderData& renderData, const World& world) {
        fillAtomRenderViews(renderData, world.getAtomStorage());
        fillAtomPalette(renderData);

        renderData.worldSize = toGlmVec3(world.getWorldSize());
        renderData.renderOffset = toGlmVec3(world.getRenderOffset());

        fillBondRenderData(renderData, world);
        fillGridRenderData(renderData, world);
    }

    inline void syncRendererWithSimulation(BaseRenderer& renderer, const Simulation& simulation) {
        while (renderer.getRenderDataCount() < simulation.worldCount()) {
            renderer.addRenderData();
        }

        for (Simulation::WorldId worldId = 0; worldId < simulation.worldCount(); ++worldId) {
            fillRenderDataFromWorld(renderer.getRenderData(worldId), simulation.worldAt(worldId));
        }

        if (simulation.worldCount() > 0) {
            const World& activeWorld = simulation.worldAt(simulation.activeWorldId());
            renderer.camera.setSceneBounds(toGlmVec3(activeWorld.getWorldSize()), toGlmVec3(activeWorld.getRenderOffset()));
        }
    }
}