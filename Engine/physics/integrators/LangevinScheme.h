#pragma once

class AtomStorage;
class Bond;
class ForceField;
class NeighborList;
class SimBox;

#include "../Bond.h"

class LangevinScheme {
public:
    void pipeline(AtomStorage& atomStorage, Bond::List& bonds, SimBox& box, ForceField& forceField, NeighborList& neighborList, bool allowBondFormation, float accelDamping, float dt) const;
};
