#include "VerletScheme.h"

#include "StepOps.h"
#include "../AtomData.h"

void VerletScheme::pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, double dt) const {
    StepOps::predictAndSync(atomStorage, box, dt, &predict);
    StepOps::computeForces(atomStorage, box, forceField, dt);

    for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
        if (atomStorage.isAtomFixed(atomIndex)) {
            continue;
        }

        correct(atomStorage, atomIndex, dt);
    }
}

void VerletScheme::predict(AtomStorage& atomStorage, std::size_t atomIndex, double dt) {
    const auto props = AtomData::getProps(atomStorage.type(atomIndex));
    const float invMass = 1.0f / props.mass;
    constexpr double damping = 0.6;

    atomStorage.velX(atomIndex) += static_cast<float>(0.5 * atomStorage.prevForceX(atomIndex) * invMass * dt * damping);
    atomStorage.velY(atomIndex) += static_cast<float>(0.5 * atomStorage.prevForceY(atomIndex) * invMass * dt * damping);
    atomStorage.velZ(atomIndex) += static_cast<float>(0.5 * atomStorage.prevForceZ(atomIndex) * invMass * dt * damping);

    atomStorage.posX(atomIndex) += static_cast<float>(atomStorage.velX(atomIndex) * dt);
    atomStorage.posY(atomIndex) += static_cast<float>(atomStorage.velY(atomIndex) * dt);
    atomStorage.posZ(atomIndex) += static_cast<float>(atomStorage.velZ(atomIndex) * dt);
}

void VerletScheme::correct(AtomStorage& atomStorage, std::size_t atomIndex, double dt) {
    const auto props = AtomData::getProps(atomStorage.type(atomIndex));
    const float invMass = 1.0f / props.mass;
    constexpr double damping = 0.6;

    atomStorage.velX(atomIndex) += static_cast<float>(0.5 * atomStorage.forceX(atomIndex) * invMass * dt * damping);
    atomStorage.velY(atomIndex) += static_cast<float>(0.5 * atomStorage.forceY(atomIndex) * invMass * dt * damping);
    atomStorage.velZ(atomIndex) += static_cast<float>(0.5 * atomStorage.forceZ(atomIndex) * invMass * dt * damping);
}

