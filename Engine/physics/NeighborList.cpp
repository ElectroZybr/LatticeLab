#include "NeighborList.h"

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
    buildCounter_.reset();
    needsRebuildCounter_.reset();
    valid_ = false;
}

void NeighborList::build(const AtomStorage& atoms, SimBox& box) {
    box.grid.rebuild(
        atoms.xDataSpan(),
        atoms.yDataSpan(),
        atoms.zDataSpan()
    );

    // buildWitchPairs(atoms, box);
    buildCounter_.startStep();
    /* перестройка списка соседей */
    const SpatialGrid& grid = box.grid;
    reserveListBuffers(atoms, grid);

    const std::size_t atomCount = atoms.size();
    offsets_[0] = 0;

    // строим карту оффсетов списка соседей
    for (std::size_t index = 0; index < atomCount; ++index) {
        forEachNeighbor(grid, atoms, index, [&](std::size_t neighborIndex) {
            if (index <= neighborIndex) return;
            if (distanceSqr(atoms, index, neighborIndex) <= listRadiusSqr_) {
                offsets_[index + 1]++;
            }
        });
    }

    for (std::size_t index = 0; index < atomCount; ++index) {
        offsets_[index + 1] += offsets_[index];
    }

    neighbors_.resize(offsets_[atomCount]); // подгоняем под нужный размер

    // заполняем список и обновляем позиции атомов последнего перестроения списка
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

    buildCounter_.finishStep();
    valid_ = true;
}

bool NeighborList::needsRebuild(const AtomStorage& atoms) const {
    needsRebuildCounter_.startStep();
    const auto finishAndReturn = [&](bool value) {
        needsRebuildCounter_.finishStep();
        return value;
    };

    /* проверка на необходимость перестройки списка */
    if (!valid_) return finishAndReturn(true);
    if (atoms.size() != refPosX_.size()) return finishAndReturn(true);

    const float maxDisp = 0.5f * skin_;
    const float maxDispSqr = maxDisp * maxDisp;

    // если атом сместился более 0.5*skin, перестраиваем список
    for (std::size_t index = 0; index < atoms.size(); ++index) {
        const float dx = atoms.posX(index) - refPosX_[index];
        const float dy = atoms.posY(index) - refPosY_[index];
        const float dz = atoms.posZ(index) - refPosZ_[index];
        const float dispSqr = dx * dx + dy * dy + dz * dz;
        if (dispSqr > maxDispSqr) {
            return finishAndReturn(true);
        }
    }

    return finishAndReturn(false);
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

void NeighborList::reserveListBuffers(const AtomStorage& atoms, const SpatialGrid& grid) {
    const std::size_t prevCapacity = neighbors_.capacity();
    neighbors_.clear();
    offsets_.assign(atoms.size() + 1, 0);
    refPosX_.resize(atoms.size());
    refPosY_.resize(atoms.size());
    refPosZ_.resize(atoms.size());

    // первый build — fallback, потом реальный размер из прошлого раза
    if (prevCapacity > 0)
        neighbors_.reserve(prevCapacity);
    else
        neighbors_.reserve(atoms.size() * 64);
}