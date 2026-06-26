#include "AccelDamp.hpp"

#include <algorithm>
#include <cmath>

#include "Lattice/Engine/Consts.h"
#include "Lattice/Engine/World.h"
#include "Lattice/Engine/metrics/Profiler.h"
#include "Lattice/Engine/physics/Atom/AtomData.h"
#include "Lattice/Engine/physics/Atom/AtomStorage.h"

namespace {
    double currentTemperature(const AtomStorage& atomStorage) {
        const size_t mobileCount = atomStorage.mobileCount();
        if (mobileCount == 0) {
            return 0.0;
        }

        double kineticEnergy = 0.0;
        for (size_t i = 0; i < mobileCount; ++i) {
            const double vx = atomStorage.vx()[i];
            const double vy = atomStorage.vy()[i];
            const double vz = atomStorage.vz()[i];
            const double speedSqr = vx * vx + vy * vy + vz * vz;
            kineticEnergy += 0.5 * static_cast<double>(AtomData::getProps(atomStorage.type()[i]).mass) * speedSqr;
        }

        const double dof = 3.0 * static_cast<double>(mobileCount);
        return (kineticEnergy > Consts::Epsilon && dof > 0.0) ? (2.0 * kineticEnergy) / (dof * static_cast<double>(Units::kboltzmann)) : 0.0;
    }
}

void AccelDamp::apply(StepContext& stepContext) {
    PROFILE_SCOPE("AccelDamp::apply");

    AtomStorage& atomStorage = stepContext.world.getAtomStorage();
    const size_t mobileCount = atomStorage.mobileCount();
    if (mobileCount == 0 || t <= 0.0 || tau <= Consts::Epsilon || stepContext.dt <= 0.0f) {
        return;
    }

    const double temperatureNow = currentTemperature(atomStorage);
    if (temperatureNow <= t || temperatureNow <= Consts::Epsilon) {
        return;
    }

    const double excess = 1.0 - t / temperatureNow;
    const float damping = static_cast<float>(std::clamp(static_cast<double>(stepContext.dt) / tau * excess, 0.0, 1.0));
    if (damping <= 0.0f) {
        return;
    }

    const std::string_view integratorId = stepContext.world.getIntegrator().getIntegrator();
    const float dt = stepContext.dt;

    const float* invMass = atomStorage.invMass().data();
    float* vx = atomStorage.vx().data();
    float* vy = atomStorage.vy().data();
    float* vz = atomStorage.vz().data();

    if (integratorId == "verlet") {
        const float* fx = atomStorage.fx().data();
        const float* fy = atomStorage.fy().data();
        const float* fz = atomStorage.fz().data();
        const float* pfx = atomStorage.pfx().data();
        const float* pfy = atomStorage.pfy().data();
        const float* pfz = atomStorage.pfz().data();

        #pragma GCC ivdep
        for (size_t i = 0; i < mobileCount; ++i) {
            const float kick = 0.5f * dt * invMass[i] * damping;
            vx[i] -= (pfx[i] + fx[i]) * kick;
        }
        #pragma GCC ivdep
        for (size_t i = 0; i < mobileCount; ++i) {
            const float kick = 0.5f * dt * invMass[i] * damping;
            vy[i] -= (pfy[i] + fy[i]) * kick;
        }
        #pragma GCC ivdep
        for (size_t i = 0; i < mobileCount; ++i) {
            const float kick = 0.5f * dt * invMass[i] * damping;
            vz[i] -= (pfz[i] + fz[i]) * kick;
        }
        return;
    }

    if (integratorId == "kdk") {
        const float* fx = atomStorage.fx().data();
        const float* fy = atomStorage.fy().data();
        const float* fz = atomStorage.fz().data();

        #pragma GCC ivdep
        for (size_t i = 0; i < mobileCount; ++i) {
            const float kick = 0.5f * dt * invMass[i] * damping;
            vx[i] -= fx[i] * kick;
        }
        #pragma GCC ivdep
        for (size_t i = 0; i < mobileCount; ++i) {
            const float kick = 0.5f * dt * invMass[i] * damping;
            vy[i] -= fy[i] * kick;
        }
        #pragma GCC ivdep
        for (size_t i = 0; i < mobileCount; ++i) {
            const float kick = 0.5f * dt * invMass[i] * damping;
            vz[i] -= fz[i] * kick;
        }
    }
}
