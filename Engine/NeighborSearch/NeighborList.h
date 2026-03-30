#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "../physics/AtomStorage.h"
#include "SpatialGrid.h"
#include "../utils/RateCounter.h"

class SimBox;

class NeighborList {
public:
    NeighborList();

    void setCutoff(float cutoff);
    void setSkin(float skin);
    void setParams(float cutoff, float skin);

    void clear();
    void build(const AtomStorage& atoms, SimBox& box);
    bool needsRebuild(const AtomStorage& atoms) const;

    [[nodiscard]] uint32_t atomCount() const;
    [[nodiscard]] uint32_t pairStorageSize() const;
    [[nodiscard]] uint32_t memoryBytes() const;
    [[nodiscard]] const RateCounter& buildCounter() const { return buildCounter_; }
    [[nodiscard]] const RateCounter& needsRebuildCounter() const { return needsRebuildCounter_; }
    [[nodiscard]] float cutoff() const { return cutoff_; }
    [[nodiscard]] float skin() const { return skin_; }
    [[nodiscard]] float listRadius() const { return listRadius_; }
    [[nodiscard]] bool isValid() const { return valid_; }
    [[nodiscard]] const std::vector<std::uint32_t>& neighbors() const { return neighbors_; }
    [[nodiscard]] const std::vector<std::uint32_t>& offsets() const { return offsets_; }
    
    // Hot-path helper для записи соседей одного атома.
    inline void writeAtomNeighbors(const SpatialGrid& grid,
                                   const float* x,
                                   const float* y,
                                   const float* z,
                                   const std::uint32_t atomIndex,
                                   const float xi,
                                   const float yi,
                                   const float zi,
                                   std::vector<std::uint32_t>& outNeighbors) const {
        const auto& offsets27 = grid.neighborOffsets27();
        const int center = grid.linearCellOfAtom(atomIndex); // центральная ячейка атома i

        for (int k = 0; k < 27; ++k) {
            for (std::uint32_t neighborIndex : grid.atomsInCellByLinearIndex(center + offsets27[k])) {
                if (neighborIndex >= atomIndex) continue;
                const float dx = x[neighborIndex] - xi;
                const float dy = y[neighborIndex] - yi;
                const float dz = z[neighborIndex] - zi;
                if (dx * dx + dy * dy + dz * dz <= listRadiusSqr_) {
                    outNeighbors.emplace_back(neighborIndex);
                }
            }
        }
    }

private:
    void reserveListBuffers(const AtomStorage& atoms, const SpatialGrid& grid);

    // uint32_t - 4 байта, максимальное количество пар в NL ~ 4 млрд
    std::vector<uint32_t> neighbors_;
    std::vector<uint32_t> offsets_;

    std::vector<float> refPosX_;
    std::vector<float> refPosY_;
    std::vector<float> refPosZ_;

    float cutoff_ = 0.0f;
    float skin_ = 0.0f;
    float listRadius_ = 0.0f;
    float listRadiusSqr_ = 0.0f;
    bool valid_ = false;

    RateCounter buildCounter_;
    mutable RateCounter needsRebuildCounter_;
};
