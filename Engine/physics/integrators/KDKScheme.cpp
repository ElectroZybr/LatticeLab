#include "KDKScheme.h"

#include "StepOps.h"

void KDKScheme::pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, NeighborList* neighborList, float dt) const {
    // Kick: половина шага
    halfKick(atomStorage, dt);
    // Расчет новых позиций
    StepOps::predictAndSync(atomStorage, box, dt, &drift);
    // Расчет сил
    StepOps::computeForces(atomStorage, box, forceField, neighborList, dt);
    // Kick: вторая половина шага
    halfKick(atomStorage,  dt);
}

void KDKScheme::halfKick(AtomStorage& atomStorage, float dt) {
    const float* RESTRICT fx  = atomStorage.fxData();
    const float* RESTRICT fy  = atomStorage.fyData();
    const float* RESTRICT fz  = atomStorage.fzData();
        
    float* RESTRICT vx = atomStorage.vxData();
    float* RESTRICT vy = atomStorage.vyData();
    float* RESTRICT vz = atomStorage.vzData();

    const float* RESTRICT invMass = atomStorage.invMassData();

    for (std::size_t i = 0; i < atomStorage.mobileCount(); ++i) {

        vx[i] += 0.5f * fx[i] * invMass[i] * dt;
        vy[i] += 0.5f * fy[i] * invMass[i] * dt;
        vz[i] += 0.5f * fz[i] * invMass[i] * dt;
    }
}

void KDKScheme::drift(AtomStorage& atomStorage, float dt) {
    float* RESTRICT x = atomStorage.xData();
    float* RESTRICT y = atomStorage.yData();
    float* RESTRICT z = atomStorage.zData();

    const float* RESTRICT vx = atomStorage.vxData();
    const float* RESTRICT vy = atomStorage.vyData();
    const float* RESTRICT vz = atomStorage.vzData();

    for (std::size_t i = 0; i < atomStorage.mobileCount(); ++i) {
        x[i] += vx[i] * dt;
        y[i] += vy[i] * dt;
        z[i] += vz[i] * dt;
    }
}

