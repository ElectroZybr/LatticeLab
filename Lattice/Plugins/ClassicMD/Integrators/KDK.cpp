#include "KDK.h"

#include "Lattice/Engine/metrics/Profiler.h"
#include "Lattice/Plugins/ClassicMD/Integrators/StepOps.h"

REGISTER_INTEGRATOR(KDK)

void KDK::pipeline(StepContext& stepContext) const {
    PROFILE_SCOPE("KDK::pipeline");
    halfKick(stepContext.world.getAtomStorage(), stepContext.dt);
    StepOps::predictAndSync(stepContext, &KDK::drift);
    StepOps::computeForces(stepContext);
    halfKick(stepContext.world.getAtomStorage(), stepContext.dt);
    StepOps::applyThermostat(stepContext);
    StepOps::postProcessVelocities(stepContext);
}

void KDK::halfKick(AtomStorage& atomStorage, float dt) {
    PROFILE_SCOPE("KDK::halfKick");
    const float* RESTRICT fx = atomStorage.fxData();
    const float* RESTRICT fy = atomStorage.fyData();
    const float* RESTRICT fz = atomStorage.fzData();

    float* RESTRICT vx = atomStorage.vxData();
    float* RESTRICT vy = atomStorage.vyData();
    float* RESTRICT vz = atomStorage.vzData();

    const float* RESTRICT invMass = atomStorage.invMassData();
    const size_t mobileCount = atomStorage.mobileCount();

    for (size_t i = 0; i < mobileCount; ++i) {
        vx[i] += 0.5f * fx[i] * invMass[i] * dt;
        vy[i] += 0.5f * fy[i] * invMass[i] * dt;
        vz[i] += 0.5f * fz[i] * invMass[i] * dt;
    }
}

void KDK::drift(AtomStorage& atomStorage, float dt) {
    PROFILE_SCOPE("KDK::drift");
    float* RESTRICT x = atomStorage.xData();
    float* RESTRICT y = atomStorage.yData();
    float* RESTRICT z = atomStorage.zData();

    const float* RESTRICT vx = atomStorage.vxData();
    const float* RESTRICT vy = atomStorage.vyData();
    const float* RESTRICT vz = atomStorage.vzData();

    const size_t mobileCount = atomStorage.mobileCount();
    for (size_t i = 0; i < mobileCount; ++i) {
        x[i] += vx[i] * dt;
        y[i] += vy[i] * dt;
        z[i] += vz[i] * dt;
    }
}
