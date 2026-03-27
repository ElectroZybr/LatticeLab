#pragma once

#include "../ForceField.h"
#include "Engine/SimBox.h"

#if defined(_MSC_VER)
    #define RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
    #define RESTRICT __restrict__
#else
    #define RESTRICT
#endif

class NeighborList;

namespace StepOps {
template<typename T>
concept AtomStepFunc = requires(T fn, AtomStorage& storage, float dt) {
    { fn(storage, dt) } -> std::same_as<void>;
};

inline void confineToBox(AtomStorage& atomStorage, SimBox& box) {
    constexpr float restitution = 0.8f;
    const Vec3f max = box.end - box.start - Vec3f(1.0, 1.0, 1.0);

    auto confineAxis = [&](float& coord, float& speed, float axisMax) {
        if (coord < 0.0f) {
            coord = 0.0f;
            if (speed < 0.0f) {
                speed = -speed * restitution;
            }
        } else if (coord > axisMax) {
            coord = axisMax;
            if (speed > 0.0f) {
                speed = -speed * restitution;
            }
        }
    };

    for (std::size_t atomIndex = 0; atomIndex < atomStorage.mobileCount(); ++atomIndex) {
        confineAxis(atomStorage.posX(atomIndex), atomStorage.velX(atomIndex), static_cast<float>(max.x));
        confineAxis(atomStorage.posY(atomIndex), atomStorage.velY(atomIndex), static_cast<float>(max.y));
        confineAxis(atomStorage.posZ(atomIndex), atomStorage.velZ(atomIndex), static_cast<float>(max.z));
    }
}

inline void computeForces(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, NeighborList* neighborList, float dt) {
    forceField.compute(atomStorage, box, neighborList, dt);
}

template<typename StepFn>
requires AtomStepFunc<StepFn>
inline void predictAndSync(AtomStorage& atomStorage, SimBox& box, float dt, StepFn predictFn) {
    static std::vector<int> oldCellsX, oldCellsY, oldCellsZ;
    const size_t n = atomStorage.mobileCount();
    oldCellsX.resize(n); oldCellsY.resize(n); oldCellsZ.resize(n);
    for (size_t i = 0; i < n; ++i) {
        oldCellsX[i] = box.grid.worldToCellX(atomStorage.posX(i));
        oldCellsY[i] = box.grid.worldToCellY(atomStorage.posY(i));
        oldCellsZ[i] = box.grid.worldToCellZ(atomStorage.posZ(i));
    }

    predictFn(atomStorage, dt);
    confineToBox(atomStorage, box);

    for (size_t i = 0; i < n; ++i) {
        const int currX = box.grid.worldToCellX(atomStorage.posX(i));
        const int currY = box.grid.worldToCellY(atomStorage.posY(i));
        const int currZ = box.grid.worldToCellZ(atomStorage.posZ(i));
        if (oldCellsX[i] != currX || oldCellsY[i] != currY || oldCellsZ[i] != currZ) {
            box.grid.eraseIndex(oldCellsX[i], oldCellsY[i], oldCellsZ[i], i);
            box.grid.insertIndex(currX, currY, currZ, i);
        }
    }

    atomStorage.swapPrevCurrentForces();
    std::fill_n(atomStorage.fxData(), atomStorage.size(), 0.0f);
    std::fill_n(atomStorage.fyData(), atomStorage.size(), 0.0f);
    std::fill_n(atomStorage.fzData(), atomStorage.size(), 0.0f);
    std::fill_n(atomStorage.energyData(), atomStorage.size(), 0.0f);
}
}
