#include "Barendsen.hpp"

#include <algorithm>
#include <cmath>

#include "Lattice/Engine/Consts.h"
#include "Lattice/Engine/World.h"
#include "Lattice/Engine/metrics/Profiler.h"
#include "Lattice/Engine/physics/Atom/AtomData.h"
#include "Lattice/Engine/physics/Atom/AtomStorage.h"

REGISTER_THERMOSTAT(Barendsen)

void Barendsen::apply(StepContext& stepContext) {
    PROFILE_SCOPE("Barendsen::apply");

    AtomStorage& atomStorage = stepContext.world.getAtomStorage();
    const size_t mobileCount = atomStorage.mobileCount();
    if (mobileCount == 0 || t <= 0.0 || nu <= Consts::Epsilon || stepContext.dt <= 0.0f) {
        return;
    }

    double kineticEnergy = 0.0;
    for (size_t i = 0; i < mobileCount; ++i) {
        const double vx = atomStorage.velX(i);
        const double vy = atomStorage.velY(i);
        const double vz = atomStorage.velZ(i);
        const double speedSqr = vx * vx + vy * vy + vz * vz;
        kineticEnergy += 0.5 * static_cast<double>(AtomData::getProps(atomStorage.type(i)).mass) * speedSqr;
    }

    const double dof = 3.0 * static_cast<double>(mobileCount);
    if (kineticEnergy <= Consts::Epsilon || dof <= 0.0) {
        return;
    }

    const double currentTemperature = (2.0 * kineticEnergy) / (dof * static_cast<double>(Units::kboltzmann));
    if (currentTemperature <= Consts::Epsilon) {
        return;
    }

    const double coupling = std::clamp(static_cast<double>(stepContext.dt) / nu, 0.0, 1.0);
    const double lambdaSqr = 1.0 + coupling * (t / currentTemperature - 1.0);
    if (lambdaSqr <= 0.0) {
        return;
    }

    const float lambda = static_cast<float>(std::sqrt(lambdaSqr));
    for (size_t i = 0; i < mobileCount; ++i) {
        atomStorage.velX(i) *= lambda;
        atomStorage.velY(i) *= lambda;
        atomStorage.velZ(i) *= lambda;
    }
}
