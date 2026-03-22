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

void VerletScheme::pipeline(AtomStorage& atomStorage, std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const {
    // Расчет новых позиций
    StepOps::predictAndSync(atoms, box, dt, &predict);
    // Расчет сил через SoA-путь
    StepOps::syncToAtomStorage(atoms, atomStorage);
    StepOps::computeForces(atomStorage, atoms, box, forceField, dt);
    // Корректировка скоростей в SoA
    for (std::size_t atomIndex = 0; atomIndex < atomStorage.size(); ++atomIndex) {
        if (!atomStorage.isAtomFixed(atomIndex)) {
            correct(atomStorage, atomIndex, dt);
        }
    }
    // Возвращаем обновленные данные обратно в AoS
    StepOps::syncFromAtomStorage(atomStorage, atoms);
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

void VerletScheme::correct(AtomStorage& atomStorage, std::size_t atomIndex, double dt) {
    const auto& props = Atom::getProps(atomStorage.type(atomIndex));
    const float invMass = 1.0f / static_cast<float>(props.mass);

    const float ax = atomStorage.forceX(atomIndex) * invMass;
    const float ay = atomStorage.forceY(atomIndex) * invMass;
    const float az = atomStorage.forceZ(atomIndex) * invMass;

    const float prevAx = atomStorage.prevForceX(atomIndex) * invMass;
    const float prevAy = atomStorage.prevForceY(atomIndex) * invMass;
    const float prevAz = atomStorage.prevForceZ(atomIndex) * invMass;

    const float halfDt = static_cast<float>(0.5 * dt);
    atomStorage.velX(atomIndex) += (prevAx + ax) * halfDt;
    atomStorage.velY(atomIndex) += (prevAy + ay) * halfDt;
    atomStorage.velZ(atomIndex) += (prevAz + az) * halfDt;
}
