#pragma once

#include <cstddef>
class World;
#include "Engine/physics/ForceFields/BondForceField.h"
#include "Engine/physics/ForceFields/CoulombForceField.h"
#include "Engine/physics/ForceFields/LJForceField.h"
#include "Engine/physics/ForceFields/WallForceField.h"

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
