#include "Integrator.h"

#include "AtomStorage.h"

Integrator::Integrator()
    : integrator_type(Scheme::Verlet),
      scheme_impl(makeSchemeImpl(Scheme::Verlet)) {}

void Integrator::setScheme(Scheme scheme) {
    integrator_type = scheme;
    scheme_impl = makeSchemeImpl(scheme);
}

void Integrator::step(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, double dt) const {
    std::visit([&](const auto& scheme) {
        scheme.pipeline(atomStorage, box, forceField, dt);
    }, scheme_impl);
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
