#pragma once

#include <array>
#include <cstddef>

#include "../math/Vec3f.h"
#include "AtomData.h"
#include "AtomStorage.h"

class SimBox;

class ForceField {
public:
    ForceField();

    void compute(AtomStorage& atoms, SimBox& box, float dt) const;
    void updateBoxCache(const SimBox& box);

    void setGravity(Vec3f gravity = Vec3f(0, 5, 0)) { static_force = gravity; }
    Vec3f getGravity() const { return static_force; }

private:
    struct LJParams {
        float forceC6 = 0.0f;      // 24 * eps * a^6
        float forceC12 = 0.0f;     // 48 * eps * a^12
        float potentialC6 = 0.0f;  // 4 * eps * a^6
        float potentialC12 = 0.0f; // 4 * eps * a^12
    };

    static constexpr std::size_t TypeCount = static_cast<std::size_t>(AtomData::Type::COUNT);
    using LJPairTable = std::array<std::array<LJParams, TypeCount>, TypeCount>;
    using LJPairRow = std::array<LJParams, TypeCount>;

    static LJPairTable buildLJPairTable();

    static void applyWall(float coord, float& force, float min, float max);
    void softWalls(const AtomStorage& atoms, std::size_t atomIndex, float& forceX, float& forceY, float& forceZ) const;
    void ComputeForces(AtomStorage& atoms, std::size_t atomIndex, SimBox& box) const;
    void pairNonBondedInteraction(AtomStorage& atoms, std::size_t bIndex, const LJPairRow& ljPairRow, float& forceX, float& forceY, float& forceZ, float posX, float posY, float posZ, float& potenE) const;
    void applyGravityForce(float& forceX, float& forceY, float& forceZ) const;

    Vec3f static_force;
    LJPairTable ljPairTable;
    float wallMinX = 0.0f;
    float wallMinY = 0.0f;
    float wallMinZ = 0.0f;
    float wallMaxX = 0.0f;
    float wallMaxY = 0.0f;
    float wallMaxZ = 0.0f;
};
