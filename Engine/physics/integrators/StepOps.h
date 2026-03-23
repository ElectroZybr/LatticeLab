#pragma once

#include <vector>

#include "../Atom.h"
#include "../ForceField.h"
#include "../../SimBox.h"

namespace StepOps {
using AtomStorageStepFn = void (*)(AtomStorage& atomStorage, std::size_t atomIndex, double dt);

inline void computeForces(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, double dt) {
    forceField.compute(atomStorage, box, dt);
}

inline void syncToAtomStorage(const std::vector<Atom>& atoms, AtomStorage& atomStorage) {
    if (atomStorage.size() != atoms.size()) {
        return;
    }

    for (std::size_t i = 0; i < atoms.size(); ++i) {
        const Atom& atom = atoms[i];
        atomStorage.setFixed(i, atom.isFixed);
    }
}

inline void syncFromAtomStorage(const AtomStorage& atomStorage, std::vector<Atom>& atoms) {
    (void)atomStorage;
    (void)atoms;
}

inline void predictAndSync(AtomStorage& atomStorage, SimBox& box, double dt, AtomStorageStepFn predictFn) {
    auto& grid = box.grid;
    const std::size_t atomCount = atomStorage.size();

    for (std::size_t atomIndex = 0; atomIndex < atomCount; ++atomIndex) {
        const int prevX = grid.worldToCellX(atomStorage.posX(atomIndex));
        const int prevY = grid.worldToCellY(atomStorage.posY(atomIndex));
        const int prevZ = grid.worldToCellZ(atomStorage.posZ(atomIndex));

        if (!atomStorage.isAtomFixed(atomIndex)) {
            predictFn(atomStorage, atomIndex, dt);
        }

        const int currX = grid.worldToCellX(atomStorage.posX(atomIndex));
        const int currY = grid.worldToCellY(atomStorage.posY(atomIndex));
        const int currZ = grid.worldToCellZ(atomStorage.posZ(atomIndex));

        if (prevX != currX || prevY != currY || prevZ != currZ) {
            grid.eraseIndex(prevX, prevY, prevZ, atomIndex);
            grid.insertIndex(currX, currY, currZ, atomIndex);
        }

        atomStorage.prevForceX(atomIndex) = atomStorage.forceX(atomIndex);
        atomStorage.prevForceY(atomIndex) = atomStorage.forceY(atomIndex);
        atomStorage.prevForceZ(atomIndex) = atomStorage.forceZ(atomIndex);

        atomStorage.forceX(atomIndex) = 0.0f;
        atomStorage.forceY(atomIndex) = 0.0f;
        atomStorage.forceZ(atomIndex) = 0.0f;
        atomStorage.energy(atomIndex) = 0.0f;
    }
}
}
