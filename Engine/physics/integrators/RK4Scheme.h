#pragma once

class AtomStorage;
class ForceField;
class NeighborList;
class SimBox;

class RK4Scheme {
public:
    void pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, NeighborList* neighborList, float dt) const;
};
