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
    constexpr float damping = 0.6f;

    const float accX = atomStorage.forceX(atomIndex) * invMass;
    const float accY = atomStorage.forceY(atomIndex) * invMass;
    const float accZ = atomStorage.forceZ(atomIndex) * invMass;

    atomStorage.posX(atomIndex) += (atomStorage.velX(atomIndex) * damping + accX * 0.5f * dt) * dt;
    atomStorage.posY(atomIndex) += (atomStorage.velY(atomIndex) * damping + accY * 0.5f * dt) * dt;
    atomStorage.posZ(atomIndex) += (atomStorage.velZ(atomIndex) * damping + accZ * 0.5f * dt) * dt;
}

void VerletScheme::correct(AtomStorage& atomStorage, std::size_t atomIndex, float dt) {
    const auto props = AtomData::getProps(atomStorage.type(atomIndex));
    const float invMass = 1.0f / props.mass;

    const float accX = atomStorage.forceX(atomIndex) * invMass;
    const float accY = atomStorage.forceY(atomIndex) * invMass;
    const float accZ = atomStorage.forceZ(atomIndex) * invMass;

    const float prevAccX = atomStorage.prevForceX(atomIndex) * invMass;
    const float prevAccY = atomStorage.prevForceY(atomIndex) * invMass;
    const float prevAccZ = atomStorage.prevForceZ(atomIndex) * invMass;

    atomStorage.velX(atomIndex) += (prevAccX + accX) * 0.5f * dt;
    atomStorage.velY(atomIndex) += (prevAccY + accY) * 0.5f * dt;
    atomStorage.velZ(atomIndex) += (prevAccZ + accZ) * 0.5f * dt;
}
