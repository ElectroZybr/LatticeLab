#include "VerletScheme.h"

#include "StepOps.h"
#include "../AtomData.h"

void VerletScheme::pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, float dt) const {
    // Расчет новых позиций
    StepOps::predictAndSync(atomStorage, box, dt, &predict);
    // Расчет сил
    StepOps::computeForces(atomStorage, box, forceField, dt);
    // Корректировка скоростей
    for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
        if (!atomStorage.isAtomFixed(atomIndex))
            correct(atomStorage, atomIndex, dt);
    }
}

void VerletScheme::predict(AtomStorage& atomStorage, std::size_t atomIndex, float dt) {
    const auto props = AtomData::getProps(atomStorage.type(atomIndex));
    const float invMass = 1.0f / props.mass;

    atomStorage.velX(atomIndex) += static_cast<float>(0.5f * atomStorage.prevForceX(atomIndex) * invMass * dt);
    atomStorage.velY(atomIndex) += static_cast<float>(0.5f * atomStorage.prevForceY(atomIndex) * invMass * dt);
    atomStorage.velZ(atomIndex) += static_cast<float>(0.5f * atomStorage.prevForceZ(atomIndex) * invMass * dt);

    atomStorage.posX(atomIndex) += static_cast<float>(atomStorage.velX(atomIndex) * dt);
    atomStorage.posY(atomIndex) += static_cast<float>(atomStorage.velY(atomIndex) * dt);
    atomStorage.posZ(atomIndex) += static_cast<float>(atomStorage.velZ(atomIndex) * dt);
}

void VerletScheme::correct(AtomStorage& atomStorage, std::size_t atomIndex, float dt) {
    const auto props = AtomData::getProps(atomStorage.type(atomIndex));
    const float invMass = 1.0f / props.mass;

    atomStorage.velX(atomIndex) += static_cast<float>(0.5f * atomStorage.forceX(atomIndex) * invMass * dt);
    atomStorage.velY(atomIndex) += static_cast<float>(0.5f * atomStorage.forceY(atomIndex) * invMass * dt);
    atomStorage.velZ(atomIndex) += static_cast<float>(0.5f * atomStorage.forceZ(atomIndex) * invMass * dt);
}
