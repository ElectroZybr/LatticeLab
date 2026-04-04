#include "ForceField.h"

#include "Engine/metrics/Profiler.h"

ForceField::ForceField() = default;

void ForceField::syncWalls(const SimBox& box) {
    wallForceField_.syncWalls(box);
}

void ForceField::compute(AtomStorage& atoms, Bond::List& bonds, SimBox& box,
                         NeighborList& neighborList, bool allowBondFormation, float dt) const {
    PROFILE_SCOPE("ForceField::compute");

    wallForceField_.compute(atoms, static_force_);
    ljForceField_.compute(atoms, neighborList);
    coulombForceField_.compute(atoms, box, neighborList);
    bondForceField_.compute(atoms, bonds, box, neighborList, allowBondFormation, dt);
}
