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

void VerletScheme::predict(AtomStorage& atomStorage, float dt) {
    const AtomData::Type* RESTRICT types = atomStorage.atomTypeData();

    float* RESTRICT x = atomStorage.xData();
    float* RESTRICT y = atomStorage.yData();
    float* RESTRICT z = atomStorage.zData();

    const float* RESTRICT fx  = atomStorage.fxData();
    const float* RESTRICT fy  = atomStorage.fyData();
    const float* RESTRICT fz  = atomStorage.fzData();
        
    const float* RESTRICT vx = atomStorage.vxData();
    const float* RESTRICT vy = atomStorage.vyData();
    const float* RESTRICT vz = atomStorage.vzData();

    for (std::size_t i = 0; i < atomStorage.mobileCount(); ++i) {
        const float invMass = 1.0f / AtomData::getProps(types[i]).mass;
        constexpr float damping = 0.6f;

        x[i] += (vx[i] * damping + fx[i] * invMass * 0.5f * dt) * dt;
        y[i] += (vy[i] * damping + fy[i] * invMass * 0.5f * dt) * dt;
        z[i] += (vz[i] * damping + fz[i] * invMass * 0.5f * dt) * dt;
    }
}

void VerletScheme::correct(AtomStorage& atomStorage, float dt) {
    const AtomData::Type* RESTRICT types = atomStorage.atomTypeData();

    const float* RESTRICT fx  = atomStorage.fxData();
    const float* RESTRICT fy  = atomStorage.fyData();
    const float* RESTRICT fz  = atomStorage.fzData();

    const float* RESTRICT pfx = atomStorage.pfxData();    
    const float* RESTRICT pfy = atomStorage.pfyData();
    const float* RESTRICT pfz = atomStorage.pfzData();
    
    float* RESTRICT vx = atomStorage.vxData();
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
