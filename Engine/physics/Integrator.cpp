#include "Integrator.h"

#include <algorithm>

#include "AtomStorage.h"
#include "integrators/StepOps.h"

Integrator::Integrator()
    : integrator_type(Scheme::Verlet),
      scheme_impl(makeSchemeImpl(Scheme::Verlet)) {}

void Integrator::setScheme(Scheme scheme) {
    integrator_type = scheme;
    scheme_impl = makeSchemeImpl(scheme);
}

void Integrator::setMaxParticleSpeed(float maxSpeed) {
    maxParticleSpeed_ = std::max(0.0f, maxSpeed);
}

void Integrator::setAccelDamping(float accelDamping) {
    accelDamping_ = std::clamp(accelDamping, 0.0f, 1.0f);
}

void Integrator::step(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, NeighborList* neighborList, float dt) {
    std::visit([&](const auto& scheme) {
        scheme.pipeline(atomStorage, box, forceField, neighborList, accelDamping_, dt);
    }, scheme_impl);
    // Ограничение максимальной скорости атомов
    if (maxParticleSpeed_ > 0.0f) {
        StepOps::postProcessVelocities(atomStorage, maxParticleSpeed_);
    }
}

Integrator::SchemeVariant Integrator::makeSchemeImpl(Scheme scheme) {
    switch (scheme) {
    case Scheme::Verlet:
        return VerletScheme{};
    case Scheme::KDK:
        return KDKScheme{};
    case Scheme::RK4:
        return RK4Scheme{};
    case Scheme::Langevin:
        return LangevinScheme{};
    default:
        return VerletScheme{};
    }
}
