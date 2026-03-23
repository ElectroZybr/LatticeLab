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

void KDKScheme::pipeline(AtomStorage& atomStorage, std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const {
    // Один раз синхронизируем AoS -> SoA в начале шага
    StepOps::syncToAtomStorage(atoms, atomStorage);
    // Kick: половина шага в SoA
    for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
        if (!atomStorage.isAtomFixed(atomIndex)) {
            halfKick(atomStorage, atomIndex, dt);
        }
    }
    // Расчет новых позиций уже в SoA
    StepOps::predictAndSync(atomStorage, atoms, box, dt, &drift);
    // Расчет сил через SoA-путь
    StepOps::computeForces(atomStorage, atoms, box, forceField, dt);
    // Kick: вторая половина шага в SoA
    for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
        if (!atomStorage.isAtomFixed(atomIndex)) {
            halfKick(atomStorage, atomIndex, dt);
        }
    }
    // Возвращаем обновленные данные обратно в AoS
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

void KDKScheme::drift(AtomStorage& atomStorage, std::size_t atomIndex, double dt) {
    const float dtf = static_cast<float>(dt);
    atomStorage.posX(atomIndex) += atomStorage.velX(atomIndex) * dtf;
    atomStorage.posY(atomIndex) += atomStorage.velY(atomIndex) * dtf;
    atomStorage.posZ(atomIndex) += atomStorage.velZ(atomIndex) * dtf;
}
