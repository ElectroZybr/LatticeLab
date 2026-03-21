#include "VerletScheme.h"

#include "../Integrator.h"
#include "../Atom.h"

void VerletScheme::pipeline(StepContext& ctx) const {
    // расчет новых позиций + проверка Grid
    ctx.integrator.predictAndSync(ctx, &predict);
    // расчет сил
    ctx.integrator.computeForces(ctx);
    // корректировка скоростей
    for (Atom& atom : ctx.atoms) {
        if (!atom.isFixed) {
            correct(atom, ctx.dt);
        }
    }
}

void VerletScheme::predict(Atom& atom, double dt) {
    constexpr float damping = 0.6f;
    const Vec3D acceleration = atom.force / atom.getProps().mass;
    atom.coords += (atom.speed * damping + acceleration * 0.5f * dt) * dt;
}

void VerletScheme::correct(Atom& atom, double dt) {
    const Vec3D acceleration = atom.force / atom.getProps().mass;
    const Vec3D prevAcceleration = atom.prev_force / atom.getProps().mass;
    atom.speed += (prevAcceleration + acceleration) * 0.5f * dt;
}