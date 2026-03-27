#include "Integrator.h"

#include "AtomStorage.h"

Integrator::Integrator()
    : integrator_type(Scheme::Verlet),
      scheme_impl(makeSchemeImpl(Scheme::Verlet)) {}

void Integrator::setScheme(Scheme scheme) {
    integrator_type = scheme;
    scheme_impl = makeSchemeImpl(scheme);
}

void Integrator::step(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, NeighborList* neighborList, float dt) {
    metrics_.onStepStart();
    std::visit([&](const auto& scheme) {
        scheme.pipeline(atomStorage, box, forceField, neighborList, dt);
    }, scheme_impl);
    metrics_.onStepFinish();
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
