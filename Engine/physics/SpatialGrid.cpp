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
    atomsInCells.clear();
    rebuildNeighborOffsets();
}

void SpatialGrid::rebuild(std::span<const float> posX,
                          std::span<const float> posY,
                          std::span<const float> posZ) {
    rebuildCounter_.startStep();

    const std::size_t n = posX.size();
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

    std::vector<std::size_t> counts(countCells, 0);
    std::size_t nonEmptyCellCount = 0;
    std::size_t maxAtomsPerCell = 0;

    for (std::size_t i = 0; i < n; ++i) {
        const int cx = worldToCellX(posX[i]);
        const int cy = worldToCellY(posY[i]);
        const int cz = worldToCellZ(posZ[i]);
        const std::size_t cellIndex = static_cast<std::size_t>(index(cx, cy, cz));
        std::size_t& counter = counts[cellIndex];
        ++counter;
        if (counter == 1) {
            ++nonEmptyCellCount;
        }
        if (counter > maxAtomsPerCell) {
            maxAtomsPerCell = counter;
        }
    }

    offsets.resize(countCells + 1);
    offsets[0] = 0;
    for (std::size_t cell = 0; cell < countCells; ++cell) {
        offsets[cell + 1] = offsets[cell] + counts[cell];
    }

    atomsInCells.resize(n);
    std::vector<std::size_t> writePtr(offsets.begin(), offsets.end() - 1);
    for (std::size_t i = 0; i < n; ++i) {
        const int cx = worldToCellX(posX[i]);
        const int cy = worldToCellY(posY[i]);
        const int cz = worldToCellZ(posZ[i]);
        const std::size_t cell = static_cast<std::size_t>(index(cx, cy, cz));
        atomsInCells[writePtr[cell]++] = i;
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
