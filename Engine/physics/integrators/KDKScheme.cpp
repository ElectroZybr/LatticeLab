#include "KDKScheme.h"

#include "StepOps.h"
#include "../Atom.h"

void KDKScheme::pipeline(std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const {
    // Kick: половина шага
    for (Atom& atom : atoms) {
        if (!atom.isFixed) {
            halfKick(atom, dt);
        }
    }
    // Расчет новых позиций
    StepOps::predictAndSync(atoms, box, dt, &drift);
    // Расчет сил
    StepOps::computeForces(atoms, box, forceField, dt);
    // Kick: вторая половина шага
    for (Atom& atom : atoms) {
        if (!atom.isFixed) {
            halfKick(atom, dt);
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
