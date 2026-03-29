#include "SpatialGrid.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

SpatialGrid::SpatialGrid(int sizeX, int sizeY, int sizeZ, int cellSize)
    : sizeX(sizeX + kBorderCells),
      sizeY(sizeY + kBorderCells),
      sizeZ(sizeZ + kBorderCells),
      cellSize(cellSize) {
    if (sizeX < 0 || sizeY < 0 || sizeZ < 0) {
        throw std::invalid_argument("SpatialGrid::SpatialGrid: invalid arguments");
    }
    countCells = this->sizeX * this->sizeY * this->sizeZ;
    offsets.assign(countCells + 1, 0);
    rebuildNeighborOffsets();
    counts_.assign(countCells, 0);
    cellIndices_.reserve(1024);
    atomsInCells.reserve(1024);
}

void SpatialGrid::rebuild(std::span<const float> posX,
                          std::span<const float> posY,
                          std::span<const float> posZ) {
    rebuildCounter_.startStep();

    const size_t n = posX.size();
    if (n != posY.size() || n != posZ.size()) {
        throw std::invalid_argument("SpatialGrid::rebuild: inconsistent coordinate span sizes");
    }

    if (n == 0) {
        atomsInCells.clear();
        std::fill(offsets.begin(), offsets.end(), 0);
        rebuildCounter_.finishStep();
        metrics_.onRebuild(rebuildCounter_.lastMs(), 0, 0, 0);
        return;
    }

    cellIndices_.resize(n);
    counts_.assign(countCells, 0);

    size_t maxAtomsPerCell = 0;
    for (size_t i = 0; i < n; ++i) {
        const size_t cell = static_cast<size_t>(
            index(worldToCellX(posX[i]), worldToCellY(posY[i]), worldToCellZ(posZ[i]))
        );
        cellIndices_[i] = cell;
        ++counts_[cell];
        maxAtomsPerCell = std::max(counts_[cell], maxAtomsPerCell);
    }

    offsets.resize(countCells + 1);
    offsets[0] = 0;
    size_t nonEmptyCellCount = 0;
    for (size_t cell = 0; cell < countCells; ++cell) {
        nonEmptyCellCount += static_cast<size_t>(counts_[cell] > 0);
        offsets[cell + 1] = offsets[cell] + counts_[cell];
    }

    std::copy(offsets.begin(), offsets.begin() + countCells, counts_.begin());
    atomsInCells.resize(n);
    for (size_t i = 0; i < n; ++i) {
        const size_t cell = cellIndices_[i];
        atomsInCells[counts_[cell]++] = i;
    }

    rebuildCounter_.finishStep();
    metrics_.onRebuild(rebuildCounter_.lastMs(), n, nonEmptyCellCount, maxAtomsPerCell);
}

void SpatialGrid::resize(int newSizeX, int newSizeY, int newSizeZ, int newCellSize) {
    if (newSizeX < 0 || newSizeY < 0 || newSizeZ < 0) {
        throw std::invalid_argument("SpatialGrid::resize: invalid arguments");
    }

    if (newCellSize > 0) cellSize = newCellSize;
    sizeX = newSizeX + kBorderCells;
    sizeY = newSizeY + kBorderCells;
    sizeZ = newSizeZ + kBorderCells;

    countCells = sizeX * sizeY * sizeZ;
    offsets.assign(countCells + 1, 0);
    atomsInCells.clear();
    rebuildNeighborOffsets();
    rebuildCounter_.reset();
    metrics_.reset();
}

void SpatialGrid::rebuildNeighborOffsets() noexcept {
    /* построение массива смещений для 27 соседей */
    const int strideX = 1;
    const int strideY = sizeX;
    const int strideZ = sizeX * sizeY;

    int k = 0;
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                neighborOffsets27_[k++] = dx * strideX + dy * strideY + dz * strideZ;
            }
        }
    }
}
