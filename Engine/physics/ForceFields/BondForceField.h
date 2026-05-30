#pragma once

#include <cstdint>

#include "Engine/physics/AtomStorage.h"
#include "Engine/physics/Bond.h"

class NeighborList;

class BondForceField {
public:
    bool compute(AtomStorage& atoms, Bond::List& bonds, const NeighborList& neighborList, bool allowBondFormation, float dt) const;

private:
    bool formBonds(AtomStorage& atoms, Bond::List& bonds, const NeighborList& neighborList) const;
    bool tryCreateBond(AtomStorage& atoms, Bond::List& bonds, uint32_t aIndex, uint32_t bIndex) const;
    static void applyAngleForces(AtomStorage& atoms, const Bond::List& bonds);
};
