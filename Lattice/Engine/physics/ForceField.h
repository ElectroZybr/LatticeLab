#pragma once

#include <cstddef>
class World;
#include "Lattice/Engine/physics/ForceFields/BondForceField.h"
#include "Lattice/Engine/physics/ForceFields/CoulombForceField.h"
#include "Lattice/Engine/physics/ForceFields/LJForceField.h"
#include "Lattice/Engine/physics/ForceFields/WallForceField.h"

class NeighborList;

class ForceField {
public:
    ForceField();

    bool compute(World& world, bool allowBondFormation, float dt) const;
    void computePairInteractions(World& world) const;

private:
    WallForceField wallForceField_;
    LJForceField ljForceField_;
    BondForceField bondForceField_;
    CoulombForceField coulombForceField_;
};
