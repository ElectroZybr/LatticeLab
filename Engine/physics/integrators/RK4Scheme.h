#pragma once

#include <vector>

class Atom;
class ForceField;
class SimBox;

class RK4Scheme {
public:
    void pipeline(std::vector<Atom>& atoms, SimBox& box, ForceField& forceField, double dt) const;
};
