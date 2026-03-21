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
}
