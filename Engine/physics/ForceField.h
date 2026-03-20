#pragma once

#include <vector>

class Atom;
class SimBox;

class ForceField {
public:
    void compute(std::vector<Atom>& atoms, SimBox& box, double dt) const;

private:
    static void applyWall(double& coord, double& speed, double& force, double min, double max);
    void softWalls(Atom& atom, SimBox& box, double dt) const;
    void ComputeForces(Atom& atom, SimBox& box, double dt) const;
    void pairNonBondedInteraction(Atom& a, Atom& b) const;
};
