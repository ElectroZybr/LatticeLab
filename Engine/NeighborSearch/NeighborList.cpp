#include "NeighborList.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#include "../physics/AtomStorage.h"
#include "SpatialGrid.h"
#include "../SimBox.h"

#include "restrict.h"

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
    // перестройка пространственной сетки
    box.grid.rebuild(atoms.xDataSpan(), atoms.yDataSpan(), atoms.zDataSpan());

    buildCounter_.startStep();
    const SpatialGrid& grid = box.grid;
    const std::uint32_t atomCount = static_cast<std::uint32_t>(atoms.size());

    reserveListBuffers(atoms, grid);

    offsets_[0] = 0;
    for (std::uint32_t i = 0; i < atomCount; ++i) {
        const float xi = atoms.posX(i);
        const float yi = atoms.posY(i);
        const float zi = atoms.posZ(i);
        // запись всех соседей в массив
        writeAtomNeighbors(grid, atoms, i, neighbors_);
        offsets_[i + 1] = neighbors_.size();

        refPosX_[i] = xi;
        refPosY_[i] = yi;
        refPosZ_[i] = zi;
    }

    buildCounter_.finishStep();
    valid_ = true;
}

bool NeighborList::needsRebuild(const AtomStorage& atoms) const {
    needsRebuildCounter_.startStep();
    const std::size_t nSize = atoms.size();
    if (nSize > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        needsRebuildCounter_.finishStep();
        return true;
    }
    const std::uint32_t n = static_cast<std::uint32_t>(nSize);

    if (!valid_ || n != refPosX_.size()) {
        needsRebuildCounter_.finishStep();
        return true;
    }

    const float maxDispSqr = (0.5f * skin_) * (0.5f * skin_);

    const float* RESTRICT x = atoms.xData();
    const float* RESTRICT y = atoms.yData();
    const float* RESTRICT z = atoms.zData();

    const float* RESTRICT refX = refPosX_.data();
    const float* RESTRICT refY = refPosY_.data();
    const float* RESTRICT refZ = refPosZ_.data();

    int rebuild = false;
    #pragma GCC ivdep
    for (std::uint32_t i = 0; i < n; ++i) {
        const float dx = x[i] - refX[i];
        const float dy = y[i] - refY[i];
        const float dz = z[i] - refZ[i];
        rebuild |= ((dx*dx + dy*dy + dz*dz) > maxDispSqr);
    }

    needsRebuildCounter_.finishStep();
    return rebuild;
}

std::uint32_t NeighborList::atomCount() const {
    if (offsets_.empty()) {
        return 0;
    }
    return static_cast<std::uint32_t>(offsets_.size() - 1);
}

std::uint32_t NeighborList::pairStorageSize() const {
    return static_cast<std::uint32_t>(std::min(neighbors_.size(), static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())));
}

std::pair<std::uint32_t, std::uint32_t> NeighborList::rangeFor(std::uint32_t atomIndex) const {
    const std::size_t index = static_cast<std::size_t>(atomIndex);
    if (offsets_.empty() || index + 1 >= offsets_.size()) {
        return {0, 0};
    }
    return {offsets_[index], offsets_[index + 1]};
}

std::uint32_t NeighborList::memoryBytes() const {
    return neighbors_.capacity() * sizeof(std::uint32_t)
        + offsets_.capacity() * sizeof(std::uint32_t)
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
