#include "VerletScheme.h"

#include "StepOps.h"

void VerletScheme::pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, NeighborList* neighborList, float dt) const {
    // Расчет новых позиций
    StepOps::predictAndSync(atomStorage, box, dt, &predict);
    // Расчет сил
    StepOps::computeForces(atomStorage, box, forceField, neighborList, dt);
    // Корректировка скоростей
    correct(atomStorage, dt);
}

void VerletScheme::predict(AtomStorage& atomStorage, float dt) {
    const std::size_t n = atomStorage.mobileCount();

    float* RESTRICT x = atomStorage.xData();
    float* RESTRICT y = atomStorage.yData();
    float* RESTRICT z = atomStorage.zData();

    const float* RESTRICT fx  = atomStorage.fxData();
    const float* RESTRICT fy  = atomStorage.fyData();
    const float* RESTRICT fz  = atomStorage.fzData();
        
    const float* RESTRICT vx = atomStorage.vxData();
    const float* RESTRICT vy = atomStorage.vyData();
    const float* RESTRICT vz = atomStorage.vzData();

    const float* RESTRICT invMass = atomStorage.invMassData();

    #pragma GCC ivdep
    for (std::size_t i = 0; i < n; ++i) {
        constexpr float damping = 0.6f;

        x[i] += (vx[i] * damping + fx[i] * invMass[i] * 0.5f * dt) * dt;
        y[i] += (vy[i] * damping + fy[i] * invMass[i] * 0.5f * dt) * dt;
        z[i] += (vz[i] * damping + fz[i] * invMass[i] * 0.5f * dt) * dt;
    }
}

void VerletScheme::correct(AtomStorage& atomStorage, float dt) {
    const std::size_t n = atomStorage.mobileCount();

    const float* RESTRICT fx  = atomStorage.fxData();
    const float* RESTRICT fy  = atomStorage.fyData();
    const float* RESTRICT fz  = atomStorage.fzData();

    const float* RESTRICT pfx = atomStorage.pfxData();    
    const float* RESTRICT pfy = atomStorage.pfyData();
    const float* RESTRICT pfz = atomStorage.pfzData();
    
    float* RESTRICT vx = atomStorage.vxData();
    float* RESTRICT vy = atomStorage.vyData();
    float* RESTRICT vz = atomStorage.vzData();

    const float* RESTRICT invMass = atomStorage.invMassData();

    #pragma GCC ivdep
    for (std::size_t i = 0; i < n; ++i) {
        const float halfDtInvMass = 0.5f * dt * invMass[i];

        vx[i] += (pfx[i] + fx[i]) * halfDtInvMass;
        vy[i] += (pfy[i] + fy[i]) * halfDtInvMass;
        vz[i] += (pfz[i] + fz[i]) * halfDtInvMass;
    }
}
