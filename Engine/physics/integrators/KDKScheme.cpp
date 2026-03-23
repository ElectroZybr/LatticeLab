#include "KDKScheme.h"

#include "StepOps.h"
#include "../AtomData.h"

void KDKScheme::pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, float dt) const {
    // Kick: половина шага
    for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
        if (!atomStorage.isAtomFixed(atomIndex))
            halfKick(atomStorage, atomIndex, dt);
    }
    // Расчет новых позиций
    StepOps::predictAndSync(atomStorage, box, dt, &drift);
    // Расчет сил
    StepOps::computeForces(atomStorage, box, forceField, dt);
    // Kick: вторая половина шага
    for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
        if (!atomStorage.isAtomFixed(atomIndex))
            halfKick(atomStorage, atomIndex, dt);
    }
}

void KDKScheme::halfKick(AtomStorage& atomStorage, std::size_t atomIndex, float dt) {
    const auto props = AtomData::getProps(atomStorage.type(atomIndex));
    const float invMass = 1.0f / props.mass;

    atomStorage.velX(atomIndex) += static_cast<float>(0.5 * atomStorage.forceX(atomIndex) * invMass * dt);
    atomStorage.velY(atomIndex) += static_cast<float>(0.5 * atomStorage.forceY(atomIndex) * invMass * dt);
    atomStorage.velZ(atomIndex) += static_cast<float>(0.5 * atomStorage.forceZ(atomIndex) * invMass * dt);
}

void KDKScheme::drift(AtomStorage& atomStorage, std::size_t atomIndex, float dt) {
    atomStorage.posX(atomIndex) += static_cast<float>(atomStorage.velX(atomIndex) * dt);
    atomStorage.posY(atomIndex) += static_cast<float>(atomStorage.velY(atomIndex) * dt);
    atomStorage.posZ(atomIndex) += static_cast<float>(atomStorage.velZ(atomIndex) * dt);
}

