#pragma once

#include "Engine/SimBox.h"
#include "Engine/math/Vec3f.h"
#include "Engine/physics/AtomStorage.h"
#include "Engine/physics/Bond.h"
#include "Engine/physics/ForceFields/BondForceField.h"
#include "Engine/physics/ForceFields/CoulombForceField.h"
#include "Engine/physics/ForceFields/LJForceField.h"
#include "Engine/physics/ForceFields/WallForceField.h"

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
