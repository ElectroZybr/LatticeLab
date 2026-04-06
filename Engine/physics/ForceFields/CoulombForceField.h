#pragma once

#include "Engine/SimBox.h"
#include "Engine/physics/AtomStorage.h"

class NeighborList;

class CoulombForceField {
public:
    void compute(AtomStorage& atoms, NeighborList& neighborList) const;
};
