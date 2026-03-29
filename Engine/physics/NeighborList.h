#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "AtomStorage.h"
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

    [[nodiscard]] std::size_t atomCount() const;
    [[nodiscard]] std::size_t pairStorageSize() const;
    [[nodiscard]] std::pair<std::size_t, std::size_t> rangeFor(std::size_t atomIndex) const;
    [[nodiscard]] std::size_t memoryBytes() const;
    [[nodiscard]] const RateCounter& buildCounter() const { return buildCounter_; }
    [[nodiscard]] const RateCounter& needsRebuildCounter() const { return needsRebuildCounter_; }
    [[nodiscard]] float cutoff() const { return cutoff_; }
    [[nodiscard]] float skin() const { return skin_; }
    [[nodiscard]] float listRadius() const { return listRadius_; }
    [[nodiscard]] bool isValid() const { return valid_; }

    [[nodiscard]] std::span<const std::size_t> neighborsIndices(std::size_t atomIndex) const {
        auto r = rangeFor(atomIndex);
        return std::span(neighbors_).subspan(r.first, r.second - r.first);
    }
    [[nodiscard]] const std::vector<std::size_t>& neighbors() const { return neighbors_; }
    [[nodiscard]] const std::vector<std::size_t>& offsets() const { return offsets_; }
    // Hot-path helper для сборки списка соседей одного атома.
    inline void writeAtomNeighbors(const SpatialGrid& grid, const AtomStorage& atoms, std::size_t atomIndex, std::vector<std::size_t>& outNeighbors) const {
        const float xi = atoms.posX(atomIndex);
        const float yi = atoms.posY(atomIndex);
        const float zi = atoms.posZ(atomIndex);

        const int cx = grid.worldToCellX(xi);
        const int cy = grid.worldToCellY(yi);
        const int cz = grid.worldToCellZ(zi);
        const int center = grid.linearIndex(cx, cy, cz);
        const auto& offsets27 = grid.neighborOffsets27();

        for (int k = 0; k < 27; ++k) {
            for (std::size_t neighborIndex : grid.atomsInCellByLinearIndex(center + offsets27[k])) {
                if (neighborIndex >= atomIndex) {
                    continue;
                }

                const float dx = atoms.posX(neighborIndex) - xi;
                const float dy = atoms.posY(neighborIndex) - yi;
                const float dz = atoms.posZ(neighborIndex) - zi;
                if (dx * dx + dy * dy + dz * dz <= listRadiusSqr_) {
                    outNeighbors.emplace_back(neighborIndex);
                }
            }
        }
    }

    template<typename F>
    void forEachNeighbor(const SpatialGrid& grid, const AtomStorage& atoms, std::size_t atomIndex, F&& callback) const {
        const int cx = grid.worldToCellX(atoms.posX(atomIndex));
        const int cy = grid.worldToCellY(atoms.posY(atomIndex));
        const int cz = grid.worldToCellZ(atoms.posZ(atomIndex));

        const int center = grid.linearIndex(cx, cy, cz);
        const auto& offsets27 = grid.neighborOffsets27();
        for (int k = 0; k < 27; ++k) {
            for (std::size_t neighbourIndex : grid.atomsInCellByLinearIndex(center + offsets27[k])) {
                callback(neighbourIndex);
            }
        }
    }

private:
    void reserveListBuffers(const AtomStorage& atoms, const SpatialGrid& grid);

    std::vector<std::size_t> neighbors_;
    std::vector<std::size_t> offsets_;

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
