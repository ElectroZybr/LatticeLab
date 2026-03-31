#pragma once

class AtomStorage;
class ForceField;
class NeighborList;
class SimBox;

class VerletScheme {
public:
    void pipeline(AtomStorage& atomStorage, SimBox& box, ForceField& forceField, NeighborList* neighborList, float accelDamping, float dt) const;

    static void predict(AtomStorage& atomStorage, float dt);
    static void correct(AtomStorage& atomStorage, float accelDamping, float dt);
};
