#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "../math/Vec3D.h"
#include "Atom.h"

class Atom;
class SimBox;

class ForceField {
public:
    ForceField();

    void compute(std::vector<Atom>& atoms, SimBox& box, double dt) const;

    void setGravity(const Vec3D& gravity) { static_force = gravity; }
    Vec3D getGravity() const { return static_force; }

private:
    struct LJParams {
        float a0 = 3.0f;
        float eps = 0.1f;
    };
    static constexpr std::size_t TypeCount = static_cast<std::size_t>(Atom::Type::COUNT);
    using LJPairTable = std::array<std::array<LJParams, TypeCount>, TypeCount>;

    static LJPairTable buildLJPairTable();

    static void applyWall(double& coord, double& speed, double& force, double min, double max);
    void softWalls(Atom& atom, SimBox& box) const;
    void ComputeForces(Atom& atom, SimBox& box) const;
    void pairNonBondedInteraction(Atom& a, Atom& b) const;
    void applyGravityForce(Atom& atom) const;

    Vec3D static_force;
    LJPairTable ljPairTable;
};
