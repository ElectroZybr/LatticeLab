#pragma once

#include "../ForceField.h"
#include "Engine/SimBox.h"
#include "restrict.h"

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
    predictFn(atomStorage, dt);
    confineToBox(atomStorage, box);

    atomStorage.swapPrevCurrentForces();
    std::fill_n(atomStorage.fxData(), atomStorage.size(), 0.0f);
    std::fill_n(atomStorage.fyData(), atomStorage.size(), 0.0f);
    std::fill_n(atomStorage.fzData(), atomStorage.size(), 0.0f);
    std::fill_n(atomStorage.energyData(), atomStorage.size(), 0.0f);
}
}
