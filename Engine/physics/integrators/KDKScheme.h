#pragma once

#include <cstddef>

class AtomStorage;
class Bond;
class ForceField;
class NeighborList;
class SimBox;

#include "../Bond.h"

class KDKScheme {
public:
    void pipeline(AtomStorage& atomStorage, Bond::List& bonds, SimBox& box, ForceField& forceField, NeighborList& neighborList, bool allowBondFormation, float accelDamping, float dt) const;

    static void halfKick(AtomStorage& atomStorage, float accelDamping, float dt);
    static void drift(AtomStorage& atomStorage, float dt);
};
