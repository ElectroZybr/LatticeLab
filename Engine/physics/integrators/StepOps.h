#pragma once

#include "../ForceField.h"
#include "../../SimBox.h"

namespace StepOps {
using AtomStorageStepFn = void (*)(AtomStorage& atomStorage, std::size_t atomIndex, float dt);

inline void confineToBox(AtomStorage& atomStorage, SimBox& box, std::size_t atomIndex) {
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

    confineAxis(atomStorage.posX(atomIndex), atomStorage.velX(atomIndex), static_cast<float>(max.x));
    confineAxis(atomStorage.posY(atomIndex), atomStorage.velY(atomIndex), static_cast<float>(max.y));
    confineAxis(atomStorage.posZ(atomIndex), atomStorage.velZ(atomIndex), static_cast<float>(max.z));
}

inline void computeForces(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, float dt) {
    forceField.compute(atomStorage, box, dt);
}

inline void predictAndSync(AtomStorage& atomStorage, SimBox& box, float dt, AtomStorageStepFn predictFn) {
    auto& grid = box.grid;
    const std::size_t atomCount = atomStorage.size();

    for (std::size_t atomIndex = 0; atomIndex < atomCount; ++atomIndex) {
        const int prevX = grid.worldToCellX(atomStorage.posX(atomIndex));
        const int prevY = grid.worldToCellY(atomStorage.posY(atomIndex));
        const int prevZ = grid.worldToCellZ(atomStorage.posZ(atomIndex));

        if (!atomStorage.isAtomFixed(atomIndex)) {
            predictFn(atomStorage, atomIndex, dt);
            confineToBox(atomStorage, box, atomIndex);
        }

        const int currX = grid.worldToCellX(atomStorage.posX(atomIndex));
        const int currY = grid.worldToCellY(atomStorage.posY(atomIndex));
        const int currZ = grid.worldToCellZ(atomStorage.posZ(atomIndex));

        if (prevX != currX || prevY != currY || prevZ != currZ) {
            grid.eraseIndex(prevX, prevY, prevZ, atomIndex);
            grid.insertIndex(currX, currY, currZ, atomIndex);
        }

        atomStorage.prevForceX(atomIndex) = atomStorage.forceX(atomIndex);
        atomStorage.prevForceY(atomIndex) = atomStorage.forceY(atomIndex);
        atomStorage.prevForceZ(atomIndex) = atomStorage.forceZ(atomIndex);

        atomStorage.forceX(atomIndex) = 0.0f;
        atomStorage.forceY(atomIndex) = 0.0f;
        atomStorage.forceZ(atomIndex) = 0.0f;
        atomStorage.energy(atomIndex) = 0.0f;
    }
}
}
