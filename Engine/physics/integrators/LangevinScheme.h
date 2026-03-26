#pragma once

class AtomStorage;
class ForceField;
class NeighborList;
class SimBox;

class LangevinScheme {
public:
    void pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, NeighborList* neighborList, float dt) const;
};
