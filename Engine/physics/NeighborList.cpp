#include "NeighborList.h"

#include <algorithm>
#include <cmath>

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
    valid_ = false;
}

void NeighborList::build(const AtomStorage& atoms, const SimBox& box) {
    const SpatialGrid& grid = box.grid;
    reserveListBuffers(atoms, grid);

    for (std::size_t index = 0; index < atoms.size(); ++index) {
        forEachNeighbor(grid, atoms, index, [&](std::size_t neighborIndex) {
            (void)neighborIndex;
        });
    }

    valid_ = true;
}

bool NeighborList::needsRebuild(const AtomStorage& atoms) const {
    (void)atoms;
    // TODO: implement displacement-based rebuild criterion.
    return !valid_;
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
