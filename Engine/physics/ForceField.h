#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "../math/Vec3D.h"
#include "Atom.h"
#include "AtomStorage.h"

class Atom;
class SimBox;

class ForceField {
public:
    ForceField();

    void compute(std::vector<Atom>& atoms, SimBox& box, float dt) const;
    void compute(AtomStorage& atoms, std::vector<Atom>& atomRefs, SimBox& box, float dt) const;

    void setGravity(Vec3D gravity = Vec3D(0, 5, 0)) { static_force = gravity; }
    Vec3D getGravity() const { return static_force; }

private:
    struct LJParams {
        float a0 = 3.0f;
        float eps = 0.1f;
    };
    static constexpr std::size_t TypeCount = static_cast<std::size_t>(Atom::Type::COUNT);
    using LJPairTable = std::array<std::array<LJParams, TypeCount>, TypeCount>;

    static LJPairTable buildLJPairTable();

    static void applyWall(float& coord, float& speed, float& force, float min, float max);
    void softWalls(Atom& atom, SimBox& box) const;
    void softWalls(AtomStorage& atoms, std::size_t atomIndex, SimBox& box) const;
    void ComputeForces(Atom& atom, SimBox& box) const;
    void ComputeForces(AtomStorage& atoms, std::vector<Atom>& atomRefs, std::size_t atomIndex, SimBox& box) const;
    void pairNonBondedInteraction(Atom& a, Atom& b) const;
    void pairNonBondedInteraction(AtomStorage& atoms, std::size_t aIndex, std::size_t bIndex) const;
    void applyGravityForce(Atom& atom) const;
    void applyGravityForce(AtomStorage& atoms, std::size_t atomIndex) const;

    Vec3D static_force;
    LJPairTable ljPairTable;
};
