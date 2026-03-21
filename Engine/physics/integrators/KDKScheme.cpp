#include "KDKScheme.h"

#include "../Integrator.h"
#include "../Atom.h"
#include "../SpatialGrid.h"

void KDKScheme::pipeline(StepContext& ctx) const {
    // Kick: половина шага
    for (Atom& atom : ctx.atoms) {
        if (!atom.isFixed) {
            halfKick(atom, ctx.dt);
        }
    }
    // расчет новых позиций + проверка Grid
    ctx.integrator.predictAndSync(ctx, &drift);
    // расчет сил
    ctx.integrator.computeForces(ctx);
    // Kick: вторая половина шага
    for (Atom& atom : ctx.atoms) {
        if (!atom.isFixed) {
            halfKick(atom, ctx.dt);
        }
    }
}

void KDKScheme::halfKick(Atom& atom, double dt) {
    const Vec3D acceleration = atom.force / atom.getProps().mass;
    atom.speed += acceleration * (0.5 * dt);
}

void KDKScheme::drift(Atom& atom, double dt) {
    atom.coords += atom.speed * dt;
}
