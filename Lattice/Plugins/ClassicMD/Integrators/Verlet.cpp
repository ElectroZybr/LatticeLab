#include "Verlet.h"

#include "Lattice/Engine/metrics/Profiler.h"
#include "Lattice/Plugins/ClassicMD/Integrators/StepOps.h"

REGISTER_INTEGRATOR(Verlet)

void Verlet::pipeline(StepContext& stepContext) const {
    PROFILE_SCOPE("Verlet::pipeline");
    // Расчет новых позиций
    StepOps::predictAndSync(stepContext, &Verlet::predict);
    // Расчет сил
    StepOps::computeForces(stepContext);
    // Корректировка скоростей
    Verlet::correct(stepContext.world.getAtomStorage(), stepContext.dt);
    StepOps::applyThermostat(stepContext);
    StepOps::postProcessVelocities(stepContext);
}

void Verlet::predict(AtomStorage& atomStorage, float dt) {
    const size_t n = atomStorage.mobileCount();

    float* RESTRICT x = atomStorage.xData();
    float* RESTRICT y = atomStorage.yData();
    float* RESTRICT z = atomStorage.zData();

    const float* RESTRICT fx = atomStorage.fxData();
    const float* RESTRICT fy = atomStorage.fyData();
    const float* RESTRICT fz = atomStorage.fzData();

    const float* RESTRICT vx = atomStorage.vxData();
    const float* RESTRICT vy = atomStorage.vyData();
    const float* RESTRICT vz = atomStorage.vzData();

    const float* RESTRICT invMass = atomStorage.invMassData();

    #pragma GCC ivdep
    for (size_t i = 0; i < n; ++i) {
        x[i] += (vx[i] + fx[i] * invMass[i] * 0.5f * dt) * dt;
        y[i] += (vy[i] + fy[i] * invMass[i] * 0.5f * dt) * dt;
        z[i] += (vz[i] + fz[i] * invMass[i] * 0.5f * dt) * dt;
    }
}

void Verlet::correct(AtomStorage& atomStorage, float dt) {
    PROFILE_SCOPE("Verlet::correct");
    const size_t n = atomStorage.mobileCount();

    const float* RESTRICT fx = atomStorage.fxData();
    const float* RESTRICT fy = atomStorage.fyData();
    const float* RESTRICT fz = atomStorage.fzData();

    const float* RESTRICT pfx = atomStorage.pfxData();
    const float* RESTRICT pfy = atomStorage.pfyData();
    const float* RESTRICT pfz = atomStorage.pfzData();

    float* RESTRICT vx = atomStorage.vxData();
    float* RESTRICT vy = atomStorage.vyData();
    float* RESTRICT vz = atomStorage.vzData();

    const float* RESTRICT invMass = atomStorage.invMassData();

    #pragma GCC ivdep
    for (size_t i = 0; i < n; ++i) {
        const float halfDtInvMass = 0.5f * dt * invMass[i];

        vx[i] += (pfx[i] + fx[i]) * halfDtInvMass;
        vy[i] += (pfy[i] + fy[i]) * halfDtInvMass;
        vz[i] += (pfz[i] + fz[i]) * halfDtInvMass;
    }
}
