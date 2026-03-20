#pragma once

#include <vector>

class Atom;
class SimBox;

class ForceField {
public:
    // TODO: move non-bonded/bonded/wall force calculations here from Atom/Bond.
    void compute(std::vector<Atom>& atoms, SimBox& box, double dt) const;
};
