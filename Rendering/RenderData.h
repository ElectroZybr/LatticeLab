#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <glm/glm.hpp>

#include "Engine/NeighborSearch/SpatialGrid.h"
#include "Engine/physics/AtomData.h"
#include "Engine/physics/Bond.h"

struct RenderColor {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

struct RenderAtomsView {
    size_t count = 0;

    const float* x = nullptr;
    const float* y = nullptr;
    const float* z = nullptr;

    const float* vx = nullptr;
    const float* vy = nullptr;
    const float* vz = nullptr;

    const AtomData::Type* type = nullptr;
    const float* radius = nullptr;

    [[nodiscard]] bool empty() const noexcept { return count == 0; }
    [[nodiscard]] bool hasPositions() const noexcept { return x != nullptr && y != nullptr && z != nullptr; }
    [[nodiscard]] bool hasVelocities() const noexcept { return vx != nullptr && vy != nullptr && vz != nullptr; }
    [[nodiscard]] bool hasTypes() const noexcept { return type != nullptr; }
    [[nodiscard]] bool hasRadii() const noexcept { return radius != nullptr; }
};

struct RenderBond {
    size_t aIndex = 0;
    size_t bIndex = 0;
};

struct RenderGridCell {
    glm::vec3 origin{};
    float cellSize = 1.0f;
    float atomCount = 0.0f;
};

class RenderData {
public:
    enum class SpeedColorMode : uint8_t {
        AtomColor = 0,
        GradientClassic = 1,
        GradientTurbo = 2,
    };

    RenderAtomsView atoms{};

    const Bond::List* bonds = nullptr;
    const SpatialGrid* grid = nullptr;
    std::vector<size_t> selectedAtomIndices;

    glm::vec3 worldSize{1.0f, 1.0f, 1.0f};
    glm::vec3 renderOffset{0.0f, 0.0f, 0.0f};

    bool drawGrid = false;
    bool drawBonds = false;
    bool drawBox = true;
    SpeedColorMode speedColorMode = SpeedColorMode::AtomColor;
    float speedGradientMax = 5.0f;
    float alpha = 0.05f;
};
