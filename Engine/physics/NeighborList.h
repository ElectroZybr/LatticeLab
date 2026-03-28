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
    };
    [[nodiscard]] const std::vector<std::size_t>& neighbors() const { return neighbors_; }
    [[nodiscard]] const std::vector<std::size_t>& offsets() const { return offsets_; }

private:
    template<typename F>
    /* helper функция, перебирает всех соседей атома */
    void forEachNeighbor(const SpatialGrid& grid, const AtomStorage& atoms, std::size_t atomIndex, F&& callback) const {
        const int cx = grid.worldToCellX(atoms.posX(atomIndex));
        const int cy = grid.worldToCellY(atoms.posY(atomIndex));
        const int cz = grid.worldToCellZ(atoms.posZ(atomIndex));

        for (int iz = cz - 1; iz <= cz + 1; ++iz) {
            for (int iy = cy - 1; iy <= cy + 1; ++iy) {
                for (int ix = cx - 1; ix <= cx + 1; ++ix) {
                    for (std::size_t neighbourIndex : grid.atomsInCell(ix, iy, iz)) {
                        callback(neighbourIndex);
                    }
                }
            }
        }
    }

    static inline float distanceSqr(const AtomStorage& atoms, std::size_t aIndex, std::size_t bIndex) {
        const float dx = atoms.posX(bIndex) - atoms.posX(aIndex);
        const float dy = atoms.posY(bIndex) - atoms.posY(aIndex);
        const float dz = atoms.posZ(bIndex) - atoms.posZ(aIndex);
        return dx * dx + dy * dy + dz * dz;
    }

    void reserveListBuffers(const AtomStorage& atoms, const SpatialGrid& grid);

    std::vector<std::size_t> neighbors_;
    std::vector<std::size_t> offsets_;

    // позиции атомов на момент перестройки списка
    std::vector<float> refPosX_;
    std::vector<float> refPosY_;
    std::vector<float> refPosZ_;

    float cutoff_ = 0.0f;        // радиус отсечки
    float skin_ = 0.0f;          // запас к радиусу отсечки
    float listRadius_ = 0.0f;    // общий радиус отсечки cutoff + skin
    float listRadiusSqr_ = 0.0f; // квадрат общего радиуса отсечки
    bool valid_ = false;         // действителен ли список

    // дебаг таймеры
    RateCounter buildCounter_;
    mutable RateCounter needsRebuildCounter_;
};
