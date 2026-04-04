#pragma once

#include "../AtomStorage.h"
#include "Engine/SimBox.h"

class NeighborList;

class CoulombForceField {
public:
    void compute(AtomStorage& atoms, SimBox& box, NeighborList& neighborList) const;
};
