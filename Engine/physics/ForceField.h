#pragma once

#include <array>
#include <vector>

class Atom;
class SimBox;

class ForceField {
public:
    ForceField();

    void compute(std::vector<Atom>& atoms, SimBox& box, double dt) const;

private:
    struct LJParams {
        float a0 = 3.0f;
        float eps = 0.1f;
    };
    using LJPairTable = std::array<std::array<LJParams, 118>, 118>;

    static LJPairTable buildLJPairTable();

    static void applyWall(double& coord, double& speed, double& force, double min, double max);
    void softWalls(Atom& atom, SimBox& box) const;
    void ComputeForces(Atom& atom, SimBox& box) const;
    void pairNonBondedInteraction(Atom& a, Atom& b) const;

    LJPairTable ljPairTable;
};
