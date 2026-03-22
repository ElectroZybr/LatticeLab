#pragma once

#include <vector>

#include "../Atom.h"
#include "../ForceField.h"
#include "../../SimBox.h"

namespace StepOps {
using AtomStepFn = void (*)(Atom& atom, double dt);

inline void predictAndSync(std::vector<Atom>& atoms, SimBox& box, double dt, AtomStepFn predictFn) {
    auto& grid = box.grid;

    for (Atom& atom : atoms) {
        const int prevX = grid.worldToCellX(atom.coords.x);
        const int prevY = grid.worldToCellY(atom.coords.y);
        const int prevZ = grid.worldToCellZ(atom.coords.z);

        if (!atom.isFixed) {
            predictFn(atom, dt);
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

inline void computeForces(std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) {
    forceField.compute(atoms, box, dt);
}

inline void computeForces(AtomStorage& atomStorage, std::vector<Atom>& atomRefs, SimBox& box, ForceField& forceField, double dt) {
    forceField.compute(atomStorage, atomRefs, box, dt);
}

inline void syncToAtomStorage(const std::vector<Atom>& atoms, AtomStorage& atomStorage) {
    if (atomStorage.size() != atoms.size()) {
        atomStorage.clear();
        atomStorage.reserve(atoms.size());

        for (const Atom& atom : atoms) {
            atomStorage.addAtom(atom.coords, atom.speed, atom.type, atom.isFixed);
            const std::size_t atomIndex = atomStorage.size() - 1;
            atomStorage.setForce(atomIndex, atom.force);
            atomStorage.setPrevForce(atomIndex, atom.prev_force);
            atomStorage.energy(atomIndex) = static_cast<float>(atom.potential_energy);
            atomStorage.valenceCount(atomIndex) = atom.valence;
        }

        return;
    }

    for (std::size_t i = 0; i < atoms.size(); ++i) {
        const Atom& atom = atoms[i];
        atomStorage.setPos(i, atom.coords);
        atomStorage.setVel(i, atom.speed);
        atomStorage.setForce(i, atom.force);
        atomStorage.setPrevForce(i, atom.prev_force);
        atomStorage.energy(i) = static_cast<float>(atom.potential_energy);
        atomStorage.valenceCount(i) = atom.valence;
    }
}

inline void syncFromAtomStorage(const AtomStorage& atomStorage, std::vector<Atom>& atoms) {
    const std::size_t atomCount = std::min(atomStorage.size(), atoms.size());

    for (std::size_t i = 0; i < atomCount; ++i) {
        Atom& atom = atoms[i];
        atom.coords = atomStorage.pos(i);
        atom.speed = atomStorage.vel(i);
        atom.force = atomStorage.force(i);
        atom.potential_energy = atomStorage.energy(i);
        atom.valence = atomStorage.valenceCount(i);
    }
}

inline void computeForcesViaStorage(AtomStorage& atomStorage, std::vector<Atom>& atomRefs, SimBox& box, ForceField& forceField, double dt) {
    syncToAtomStorage(atomRefs, atomStorage);
    computeForces(atomStorage, atomRefs, box, forceField, dt);
    syncFromAtomStorage(atomStorage, atomRefs);
}
}
