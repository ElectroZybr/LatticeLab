#pragma once

#include <cstddef>

class AtomStorage;
class ForceField;
class NeighborList;
class SimBox;

class KDKScheme {
public:
    void pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, NeighborList* neighborList, float dt) const;

    static void halfKick(AtomStorage& atomStorage, float dt);
    static void drift(AtomStorage& atomStorage, float dt);
};
