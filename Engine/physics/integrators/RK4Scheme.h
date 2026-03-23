#pragma once

#include <vector>

class Atom;
class AtomStorage;
class ForceField;
class SimBox;

class RK4Scheme {
public:
    void pipeline(AtomStorage& atomStorage, std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const;
};
