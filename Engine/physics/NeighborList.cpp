#include "NeighborList.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "AtomStorage.h"
#include "SpatialGrid.h"
#include "../SimBox.h"

NeighborList::NeighborList() = default;

void NeighborList::setCutoff(float cutoff) {
    cutoff_ = cutoff;
    listRadius_ = cutoff_ + skin_;
    listRadiusSqr_ = listRadius_ * listRadius_;
    valid_ = false;
}

void NeighborList::setSkin(float skin) {
    skin_ = skin;
    listRadius_ = cutoff_ + skin_;
    listRadiusSqr_ = listRadius_ * listRadius_;
    valid_ = false;
}

void NeighborList::setParams(float cutoff, float skin) {
    cutoff_ = cutoff;
    skin_ = skin;
    listRadius_ = cutoff_ + skin_;
    listRadiusSqr_ = listRadius_ * listRadius_;
    valid_ = false;
}

void NeighborList::clear() {
    neighbors_.clear();
    offsets_.clear();
    refPosX_.clear();
    refPosY_.clear();
    refPosZ_.clear();
    neighbors_.shrink_to_fit();
    offsets_.shrink_to_fit();
    refPosX_.shrink_to_fit();
    refPosY_.shrink_to_fit();
    refPosZ_.shrink_to_fit();
    valid_ = false;
}

void NeighborList::build(const AtomStorage& atoms, const SimBox& box) {
    /* перестройка списка соседей */
    const SpatialGrid& grid = box.grid;
    const std::size_t atomCount = atoms.size();

    reserveListBuffers(atoms, grid);

    std::fill(offsets_.begin(), offsets_.end(), 0);

    for (std::size_t index = 0; index < atomCount; ++index) {
        forEachNeighbor(grid, atoms, index, [&](std::size_t neighborIndex) {
            if (index <= neighborIndex) return;
            if (distanceSqr(atoms, index, neighborIndex) <= listRadiusSqr_) {
                // Временно храним количество в offsets_[index + 1]
                offsets_[index + 1]++;
            }
        });
    }

    for (std::size_t index = 0; index < atomCount; ++index) {
        offsets_[index + 1] += offsets_[index];
    }

    neighbors_.resize(offsets_[atomCount]);

    std::vector<std::size_t> currentPositions = offsets_;

    for (std::size_t index = 0; index < atomCount; ++index) {
        forEachNeighbor(grid, atoms, index, [&](std::size_t neighborIndex) {
            if (index <= neighborIndex) return;
            if (distanceSqr(atoms, index, neighborIndex) <= listRadiusSqr_) {
                std::size_t pos = currentPositions[index]++;
                neighbors_[pos] = neighborIndex;
            }
        });

        refPosX_[index] = atoms.posX(index);
        refPosY_[index] = atoms.posY(index);
        refPosZ_[index] = atoms.posZ(index);
    }

    valid_ = true;
}

bool NeighborList::needsRebuild(const AtomStorage& atoms) const {
    /* проверка на необходимость перестройки списка */
    if (!valid_) return true;
    if (atoms.size() != refPosX_.size()) return true;

    const float maxDisp = 0.5f * skin_; 
    const float maxDispSqr = maxDisp * maxDisp;

    // если атом сместился более 0.5*skin, перестраиваем список
    for (std::size_t index = 0; index < atoms.size(); ++index) {
        const float dx = atoms.posX(index) - refPosX_[index];
        const float dy = atoms.posY(index) - refPosY_[index];
        const float dz = atoms.posZ(index) - refPosZ_[index];
        const float dispSqr = dx * dx + dy * dy + dz * dz;
        if (dispSqr > maxDispSqr) {
            return true;
        }
    }

    return false;
}

std::size_t NeighborList::atomCount() const {
    if (offsets_.empty()) {
        return 0;
    }
    return offsets_.size() - 1;
}

std::size_t NeighborList::pairStorageSize() const {
    return neighbors_.size();
}

std::pair<std::size_t, std::size_t> NeighborList::rangeFor(std::size_t atomIndex) const {
    if (offsets_.empty() || atomIndex + 1 >= offsets_.size()) {
        return {0, 0};
    }
    return {offsets_[atomIndex], offsets_[atomIndex + 1]};
}

std::size_t NeighborList::memoryBytes() const {
    return neighbors_.capacity() * sizeof(std::size_t)
        + offsets_.capacity() * sizeof(std::size_t)
        + refPosX_.capacity() * sizeof(float)
        + refPosY_.capacity() * sizeof(float)
        + refPosZ_.capacity() * sizeof(float);
}

const std::vector<std::size_t>* NeighborList::getCellAtomIndices(
    const SpatialGrid& grid, int x, int y, int z) const {
    return grid.atIndex(x, y, z);
}

std::size_t NeighborList::estimateNeighborCapacity(
    const AtomStorage& atoms, const SpatialGrid& grid) const {
    /* расчет необходимого места под список соседей */
    if (atoms.empty()) return 0;

    std::size_t nonEmptyCellCount = 0;
    std::size_t totalCellOccupancy = 0;

    forEachNonEmptyCell(grid, [&](const std::vector<std::size_t>& cell) {
        ++nonEmptyCellCount;
        totalCellOccupancy += cell.size();
    });

    constexpr std::size_t kFallbackNeighboursPerAtom = 64;
    if (nonEmptyCellCount == 0) {
        return atoms.size() * kFallbackNeighboursPerAtom;
    }

    const double avgOccupancy = static_cast<double>(totalCellOccupancy) /
        static_cast<double>(nonEmptyCellCount);
    const std::size_t estimatedNeighboursPerAtom =
        std::max<std::size_t>(16, static_cast<std::size_t>(std::ceil(avgOccupancy * 12.0)));

    return atoms.size() * estimatedNeighboursPerAtom;
}

void NeighborList::reserveListBuffers(const AtomStorage& atoms, const SpatialGrid& grid) {
    /* резервирование буферов под список соседей */
    neighbors_.clear();
    offsets_.assign(atoms.size() + 1, 0);

    refPosX_.resize(atoms.size());
    refPosY_.resize(atoms.size());
    refPosZ_.resize(atoms.size());

    neighbors_.reserve(estimateNeighborCapacity(atoms, grid));
}

