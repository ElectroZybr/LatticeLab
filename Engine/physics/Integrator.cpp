#include "Integrator.h"

#include "Atom.h"
#include "ForceField.h"
#include "../SimBox.h"

Integrator::Integrator()
    : integrator_type(Scheme::Verlet),
      scheme_impl(makeSchemeImpl(Scheme::Verlet)) {}

void Integrator::setScheme(Scheme scheme) {
    integrator_type = scheme;
    scheme_impl = makeSchemeImpl(scheme);
}

void Integrator::step(std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const {
    StepContext ctx{
        .atoms = atoms,
        .box = box,
        .forceField = forceField,
        .dt = dt,
        .integrator = *this,
    };

    std::visit([&](const auto& scheme) {
        scheme.pipeline(ctx);
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

void Integrator::predictAndSync(StepContext& ctx, AtomStepFn predictFn) const {
    auto& grid = ctx.box.grid;

    for (Atom& atom : ctx.atoms) {
        const int prevX = grid.worldToCellX(atom.coords.x);
        const int prevY = grid.worldToCellY(atom.coords.y);
        const int prevZ = grid.worldToCellZ(atom.coords.z);

        if (!atom.isFixed) {
            predictFn(atom, ctx.dt);
        }

        const int currX = grid.worldToCellX(atom.coords.x);
        const int currY = grid.worldToCellY(atom.coords.y);
        const int currZ = grid.worldToCellZ(atom.coords.z);

        if (prevX != currX || prevY != currY || prevZ != currZ) {
            grid.erase(prevX, prevY, prevZ, &atom);
            grid.insert(currX, currY, currZ, &atom);
        }

        atom.prev_force = atom.force;
        atom.force = Vec3D(0, 0, 0);
        atom.potential_energy = 0.0f;
    }
}

void Integrator::computeForces(StepContext& ctx) const {
    ctx.forceField.compute(ctx.atoms, ctx.box, ctx.dt);
}
