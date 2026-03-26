#pragma once

#include <cstddef>

class AtomStorage;
class ForceField;
class NeighborList;
class SimBox;

class VerletScheme {
public:
    void pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, NeighborList* neighborList, float dt) const;

    static void predict(AtomStorage& atomStorage, std::size_t atomIndex, float dt);
    static void correct(AtomStorage& atomStorage, std::size_t atomIndex, float dt);
};
