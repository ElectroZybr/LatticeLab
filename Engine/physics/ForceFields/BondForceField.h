#pragma once

#include <cstdint>

#include "../AtomStorage.h"
#include "../Bond.h"
#include "Engine/SimBox.h"

class NeighborList;

class BondForceField {
public:
    void compute(AtomStorage& atoms, Bond::List& bonds,
                 NeighborList& neighborList, bool allowBondFormation, float dt) const;

private:
    void formBonds(AtomStorage& atoms, Bond::List& bonds, NeighborList& neighborList) const;
    void tryCreateBond(AtomStorage& atoms, Bond::List& bonds, uint32_t aIndex, uint32_t bIndex) const;
    static void applyAngleForces(AtomStorage& atoms, const Bond::List& bonds);
};
