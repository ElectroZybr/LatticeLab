#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "../AtomData.h"
#include "../AtomStorage.h"
class NeighborList;

class LJForceField {
public:
    LJForceField();

    void compute(AtomStorage& atoms, NeighborList& neighborList) const;

private:
    struct LJParams {
        float potentialC6 = 0.0f;
        float potentialC12 = 0.0f;
    };

    static constexpr size_t TypeCount = static_cast<size_t>(AtomData::Type::COUNT);
    using LJPairTable = std::array<std::array<LJParams, TypeCount>, TypeCount>;
    using LJPairRow = std::array<LJParams, TypeCount>;

    static LJPairTable buildLJPairTable();
    void pairInteraction(AtomStorage& atoms, uint32_t bIndex, const LJPairRow& ljPairRow,
                         float& forceX, float& forceY, float& forceZ,
                         float posX, float posY, float posZ, float& potenE) const;

    LJPairTable ljPairTable_;
};
