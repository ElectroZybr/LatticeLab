#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include "AtomStorage.h"
#include "SpatialGrid.h"

class SimBox;

class NeighborList {
public:
    NeighborList();

    void setCutoff(float cutoff);
    void setSkin(float skin);
    void setParams(float cutoff, float skin);

    void clear();
    void build(const AtomStorage& atoms, const SimBox& box);
    bool needsRebuild(const AtomStorage& atoms) const;

    [[nodiscard]] std::size_t atomCount() const;
    [[nodiscard]] std::size_t pairStorageSize() const;
    [[nodiscard]] std::pair<std::size_t, std::size_t> rangeFor(std::size_t atomIndex) const;

    [[nodiscard]] const std::vector<std::size_t>& neighbors() const { return neighbors_; }
    [[nodiscard]] const std::vector<std::size_t>& offsets() const { return offsets_; }

private:
    template<typename F>
    void forEachNonEmptyCell(const SpatialGrid& grid, F&& callback) const {
        for (int z = 0; z < grid.sizeZ; ++z) {
            for (int y = 0; y < grid.sizeY; ++y) {
                for (int x = 0; x < grid.sizeX; ++x) {
                    if (const auto* cell = getCellAtomIndices(grid, x, y, z); cell && !cell->empty()) {
                        callback(*cell);
                    }
                }
            }
        }
    }

    template<typename F>
    void forEachNeighbor(const SpatialGrid& grid, const AtomStorage& atoms, std::size_t atomIndex, F&& callback) const {
        const int cx = grid.worldToCellX(atoms.posX(atomIndex));
        const int cy = grid.worldToCellY(atoms.posY(atomIndex));
        const int cz = grid.worldToCellZ(atoms.posZ(atomIndex));

        const int x0 = std::max(cx - 1, 0);
        const int x1 = std::min(cx + 1, grid.sizeX - 1);
        const int y0 = std::max(cy - 1, 0);
        const int y1 = std::min(cy + 1, grid.sizeY - 1);
        const int z0 = std::max(cz - 1, 0);
        const int z1 = std::min(cz + 1, grid.sizeZ - 1);

        for (int iz = z0; iz <= z1; ++iz) {
            for (int iy = y0; iy <= y1; ++iy) {
                for (int ix = x0; ix <= x1; ++ix) {
                    if (const auto* cell = getCellAtomIndices(grid, ix, iy, iz)) {
                        for (std::size_t neighborIndex : *cell) {
                            callback(neighborIndex);
                        }
                    }
                }
            }
        }
    }

    [[nodiscard]] const std::vector<std::size_t>* getCellAtomIndices(
        const SpatialGrid& grid, int x, int y, int z) const;
    [[nodiscard]] std::size_t estimateNeighborCapacity(
        const AtomStorage& atoms, const SpatialGrid& grid) const;
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
};
