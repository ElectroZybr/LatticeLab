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
    box.grid.rebuild(atoms.xDataSpan(), atoms.yDataSpan(), atoms.zDataSpan());

    buildCounter_.startStep();
    const SpatialGrid& grid = box.grid;
    const std::size_t atomCount = atoms.size();

    reserveListBuffers(atoms, grid);

    std::vector<std::size_t> tmp;
    tmp.reserve(neighbors_.capacity());

    offsets_[0] = 0;
    for (std::size_t i = 0; i < atomCount; ++i) {
        forEachNeighbor(grid, atoms, i, [&](std::size_t j) {
            if (j >= i) return;
            if (distanceSqr(atoms, i, j) <= listRadiusSqr_)
                tmp.emplace_back(j);
        });
        offsets_[i + 1] = tmp.size();

        refPosX_[i] = atoms.posX(i);
        refPosY_[i] = atoms.posY(i);
        refPosZ_[i] = atoms.posZ(i);
    }

    neighbors_ = std::move(tmp);

    buildCounter_.finishStep();
    valid_ = true;
}

bool NeighborList::needsRebuild(const AtomStorage& atoms) const {
    needsRebuildCounter_.startStep();

    if (!valid_ || atoms.size() != refPosX_.size()) {
        needsRebuildCounter_.finishStep();
        return true;
    }

    const float maxDispSqr = (0.5f * skin_) * (0.5f * skin_);

    bool rebuild = false;
    for (std::size_t i = 0; i < atoms.size(); ++i) {
        const float dx = atoms.posX(i) - refPosX_[i];
        const float dy = atoms.posY(i) - refPosY_[i];
        const float dz = atoms.posZ(i) - refPosZ_[i];
        rebuild |= (dx*dx + dy*dy + dz*dz) > maxDispSqr;
    }

    needsRebuildCounter_.finishStep();
    return rebuild;
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