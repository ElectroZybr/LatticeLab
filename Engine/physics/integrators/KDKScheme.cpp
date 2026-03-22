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
    // –асчет новых позиций
    StepOps::predictAndSync(atoms, box, dt, &drift);
    // –асчет сил
    StepOps::computeForces(atoms, box, forceField, dt);
    // Kick: втора€ половина шага
    for (Atom& atom : atoms) {
        if (!atom.isFixed) {
            halfKick(atom, dt);
        }
    }
}

void KDKScheme::pipeline(AtomStorage& atomStorage, std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const {
    // Kick: половина шага
    for (Atom& atom : atoms) {
        if (!atom.isFixed) {
            halfKick(atom, dt);
        }
    }
    // –асчет новых позиций
    StepOps::predictAndSync(atoms, box, dt, &drift);
    // –асчет сил через SoA-путь
    StepOps::syncToAtomStorage(atoms, atomStorage);
    StepOps::computeForces(atomStorage, atoms, box, forceField, dt);
    // Kick: втора€ половина шага в SoA
    for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
        if (!atomStorage.isAtomFixed(atomIndex)) {
            halfKick(atomStorage, atomIndex, dt);
        }
    }
    // ¬озвращаем обновленные данные обратно в AoS
    StepOps::syncFromAtomStorage(atomStorage, atoms);
}

void KDKScheme::halfKick(Atom& atom, double dt) {
    const Vec3D acceleration = atom.force / atom.getProps().mass;
    atom.speed += acceleration * (0.5 * dt);
}

void KDKScheme::halfKick(AtomStorage& atomStorage, std::size_t atomIndex, double dt) {
    const auto& props = Atom::getProps(atomStorage.type(atomIndex));
    const float invMass = 1.0f / static_cast<float>(props.mass);
    const float halfDt = static_cast<float>(0.5 * dt);

    atomStorage.velX(atomIndex) += atomStorage.forceX(atomIndex) * invMass * halfDt;
    atomStorage.velY(atomIndex) += atomStorage.forceY(atomIndex) * invMass * halfDt;
    atomStorage.velZ(atomIndex) += atomStorage.forceZ(atomIndex) * invMass * halfDt;
}

void KDKScheme::drift(Atom& atom, double dt) {
    atom.coords += atom.speed * dt;
}
