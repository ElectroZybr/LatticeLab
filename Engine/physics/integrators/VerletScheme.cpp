#include "VerletScheme.h"

#include "StepOps.h"
#include "../Atom.h"

void VerletScheme::pipeline(std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const {
    // Расчет новых позиций
    StepOps::predictAndSync(atoms, box, dt, &predict);
    // Расчет сил
    StepOps::computeForces(atoms, box, forceField, dt);
    // Корректировка скоростей
    for (Atom& atom : atoms) {
        if (!atom.isFixed) {
            correct(atom, dt);
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
