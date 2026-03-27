#include "VerletScheme.h"

#include "StepOps.h"
#include "../AtomData.h"

#if defined(_MSC_VER)
    #define RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
    #define RESTRICT __restrict__
#else
    #define RESTRICT
#endif

void VerletScheme::pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, NeighborList* neighborList, float dt) const {
    // Расчет новых позиций
    StepOps::predictAndSync(atomStorage, box, dt, &predict);
    // Расчет сил
    StepOps::computeForces(atomStorage, box, forceField, neighborList, dt);
    // Корректировка скоростей
    correct(atomStorage, dt);
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

void VerletScheme::correct(AtomStorage& atomStorage, float dt) {
    const AtomData::Type* RESTRICT types = atomStorage.atomTypeData();

    const float* RESTRICT fx  = atomStorage.fxData();
    const float* RESTRICT pfx = atomStorage.pfxData();
    float* RESTRICT vx = atomStorage.vxData();

    const float* RESTRICT fy  = atomStorage.fyData();
    const float* RESTRICT pfy = atomStorage.pfyData();
    
    const float* RESTRICT fz  = atomStorage.fzData();
    const float* RESTRICT pfz = atomStorage.pfzData();

    float* RESTRICT vy = atomStorage.vyData();
    float* RESTRICT vz = atomStorage.vzData();

    for (std::size_t i = 0; i < atomStorage.mobileCount(); ++i) {
        const float invMass = 1.0f / AtomData::getProps(types[i]).mass;
        const float halfDtInvMass = 0.5f * dt * invMass;

        vx[i] += (pfx[i] + fx[i]) * halfDtInvMass;
        vy[i] += (pfy[i] + fy[i]) * halfDtInvMass;
        vz[i] += (pfz[i] + fz[i]) * halfDtInvMass;
    }
}
