#pragma once

#include "AtomStorage.h"
#include "Bond.h"

#include "Engine/SimBox.h"
#include "Engine/math/Vec3f.h"
#include "ForceFields/BondForceField.h"
#include "ForceFields/CoulombForceField.h"
#include "ForceFields/LJForceField.h"
#include "ForceFields/WallForceField.h"

class NeighborList;

class ForceField {
public:
    ForceField();

    void compute(AtomStorage& atoms, Bond::List& bonds, SimBox& box, NeighborList& neighborList, bool allowBondFormation, float dt) const;
    void syncWalls(const SimBox& box);

    void setGravity(Vec3f gravity = Vec3f(0, 5, 0)) { static_force_ = gravity; }
    Vec3f getGravity() const { return static_force_; }

private:
    Vec3f static_force_;
    WallForceField wallForceField_;
    LJForceField ljForceField_;
    BondForceField bondForceField_;
    CoulombForceField coulombForceField_;
};
