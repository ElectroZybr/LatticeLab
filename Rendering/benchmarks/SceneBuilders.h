#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "Rendering/RenderData.h"

struct RenderBenchScene {
    RenderData data{};
    std::vector<float> atomX;
    std::vector<float> atomY;
    std::vector<float> atomZ;
    std::vector<float> atomVx;
    std::vector<float> atomVy;
    std::vector<float> atomVz;
    std::vector<float> atomRadii;
    std::vector<uint8_t> atomTypes;
    std::vector<View::RenderGridCell> gridCells;
    std::vector<float> fieldValues;
    std::vector<glm::vec2> fieldVectors;
    glm::vec3 worldSize{64.0f};
    glm::vec3 renderOffset{0.0f};
    size_t workItemCount = 0;

    static void forEachGridCells(const void* context, View::RenderGridCellVisitor visitor, void* userData) {
        const auto& cells = *static_cast<const std::vector<View::RenderGridCell>*>(context);
        for (const auto& cell : cells) {
            visitor(cell, userData);
        }
    }
};

namespace RenderBenchScenes {
    inline void clear(RenderBenchScene& scene) {
        scene.atomX.clear();
        scene.atomY.clear();
        scene.atomZ.clear();
        scene.atomVx.clear();
        scene.atomVy.clear();
        scene.atomVz.clear();
        scene.atomRadii.clear();
        scene.atomTypes.clear();
        scene.gridCells.clear();
        scene.fieldValues.clear();
        scene.fieldVectors.clear();

        scene.worldSize = glm::vec3(64.0f);
        scene.renderOffset = glm::vec3(0.0f);
        scene.workItemCount = 0;
        scene.data = RenderData{};
        scene.data.isActiveWorld = true;
        scene.data.worldSize = scene.worldSize;
        scene.data.renderOffset = scene.renderOffset;
        scene.data.hasBox = false;
    }

    inline void buildAtoms(RenderBenchScene& scene, size_t atomCount) {
        clear(scene);
        scene.workItemCount = atomCount;

        scene.atomX.resize(atomCount);
        scene.atomY.resize(atomCount);
        scene.atomZ.resize(atomCount);
        scene.atomVx.resize(atomCount);
        scene.atomVy.resize(atomCount);
        scene.atomVz.resize(atomCount);
        scene.atomRadii.resize(atomCount);
        scene.atomTypes.resize(atomCount);

        const size_t side = std::max<size_t>(1, static_cast<size_t>(std::ceil(std::cbrt(static_cast<double>(atomCount)))));
        const float spacing = 2.4f;
        const glm::vec3 extent = glm::vec3(static_cast<float>(side > 0 ? side - 1 : 0)) * spacing;
        const glm::vec3 origin = -extent * 0.5f;
        scene.worldSize = glm::max(extent + glm::vec3(16.0f), glm::vec3(32.0f));

        for (size_t i = 0; i < atomCount; ++i) {
            const size_t x = i % side;
            const size_t y = (i / side) % side;
            const size_t z = i / (side * side);
            const glm::vec3 pos = origin + glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)) * spacing;

            scene.atomX[i] = pos.x;
            scene.atomY[i] = pos.y;
            scene.atomZ[i] = pos.z;
            scene.atomVx[i] = std::sin(static_cast<float>(i) * 0.013f) * 0.1f;
            scene.atomVy[i] = std::cos(static_cast<float>(i) * 0.017f) * 0.1f;
            scene.atomVz[i] = std::sin(static_cast<float>(i) * 0.019f) * 0.1f;
            scene.atomRadii[i] = 0.85f + static_cast<float>(i % 3) * 0.05f;
            scene.atomTypes[i] = static_cast<uint8_t>((i % 3) + 1);
        }

        scene.data.atoms = View::RenderAtomsView{
            .count = atomCount,
            .x = scene.atomX.data(),
            .y = scene.atomY.data(),
            .z = scene.atomZ.data(),
            .vx = scene.atomVx.data(),
            .vy = scene.atomVy.data(),
            .vz = scene.atomVz.data(),
            .type = scene.atomTypes.data(),
            .radius = scene.atomRadii.data(),
        };
        scene.data.worldSize = scene.worldSize;
        scene.data.renderOffset = scene.renderOffset;
        scene.data.drawAtoms = true;
    }

    inline void buildGrid(RenderBenchScene& scene, size_t cellCount) {
        clear(scene);

        const size_t side = std::max<size_t>(1, static_cast<size_t>(std::ceil(std::cbrt(static_cast<double>(cellCount)))));
        const float cellSize = 2.0f;
        const glm::vec3 extent = glm::vec3(static_cast<float>(side)) * cellSize;
        const glm::vec3 origin = -extent * 0.5f;

        scene.gridCells.reserve(side * side * side);
        for (size_t z = 0; z < side; ++z) {
            for (size_t y = 0; y < side; ++y) {
                for (size_t x = 0; x < side; ++x) {
                    if (scene.gridCells.size() >= cellCount) {
                        break;
                    }
                    scene.gridCells.push_back(View::RenderGridCell{
                        .origin = origin + glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)) * cellSize,
                        .cellSize = cellSize,
                        .atomCount = 1.0f + static_cast<float>((x + y + z) % 9),
                    });
                }
            }
        }

        scene.workItemCount = scene.gridCells.size();
        scene.worldSize = glm::max(extent + glm::vec3(12.0f), glm::vec3(32.0f));
        scene.data.grid = View::RenderRectGridView{
            .context = &scene.gridCells,
            .count = scene.gridCells.size(),
            .forEachFn = RenderBenchScene::forEachGridCells,
        };
        scene.data.worldSize = scene.worldSize;
        scene.data.renderOffset = scene.renderOffset;
        scene.data.drawGrid = true;
    }

    inline void buildField(RenderBenchScene& scene, size_t sampleCount, bool drawPotential, bool drawArrows, bool drawContours) {
        clear(scene);

        const int side = std::max(2, static_cast<int>(std::ceil(std::sqrt(static_cast<double>(sampleCount)))));
        const float cellSize = 1.6f;
        scene.fieldValues.resize(static_cast<size_t>(side) * static_cast<size_t>(side));
        scene.fieldVectors.resize(static_cast<size_t>(side) * static_cast<size_t>(side));

        for (int y = 0; y < side; ++y) {
            for (int x = 0; x < side; ++x) {
                const size_t index = static_cast<size_t>(x) + static_cast<size_t>(side) * static_cast<size_t>(y);
                const float fx = (static_cast<float>(x) / static_cast<float>(side - 1) - 0.5f) * 2.0f;
                const float fy = (static_cast<float>(y) / static_cast<float>(side - 1) - 0.5f) * 2.0f;
                scene.fieldValues[index] = std::sin(fx * 3.2f) * std::cos(fy * 2.7f);
                scene.fieldVectors[index] = glm::vec2(-fy, fx) * 0.45f;
            }
        }

        scene.workItemCount = scene.fieldValues.size();
        scene.worldSize = glm::vec3(static_cast<float>(side - 1) * cellSize + 16.0f, static_cast<float>(side - 1) * cellSize + 16.0f, 32.0f);
        scene.data.vectorField = View::RenderVectorFieldView{
            .values = scene.fieldValues.data(),
            .vectors = scene.fieldVectors.data(),
            .gridSize = glm::ivec2(side, side),
            .coverageSize = glm::ivec2(static_cast<int>(std::round((side - 1) * cellSize)), static_cast<int>(std::round((side - 1) * cellSize))),
            .cellSize = cellSize,
            .z = 0.0f,
        };
        scene.data.worldSize = scene.worldSize;
        scene.data.renderOffset = scene.renderOffset;
        scene.data.drawVectorField = drawPotential;
        scene.data.drawFieldArrows = drawArrows;
        scene.data.drawFieldContours = drawContours;
        scene.data.fieldAutoScale = true;
        scene.data.fieldPotentialScale = 1.0f;
        scene.data.fieldSmoothing = 0.35f;
        scene.data.fieldContourStep = 0.2f;
    }
}
