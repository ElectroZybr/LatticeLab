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

    cellIndices_.resize(n);
    counts_.assign(countCells, 0);

    for (std::size_t i = 0; i < n; ++i) {
        const std::size_t cell = static_cast<std::size_t>(
            index(worldToCellX(posX[i]), worldToCellY(posY[i]), worldToCellZ(posZ[i]))
        );
        cellIndices_[i] = cell;
        ++counts_[cell];
    }

    offsets.resize(countCells + 1);
    offsets[0] = 0;
    std::size_t nonEmptyCellCount = 0;
    std::size_t maxAtomsPerCell = 0;
    for (std::size_t cell = 0; cell < countCells; ++cell) {
        nonEmptyCellCount += static_cast<std::size_t>(counts_[cell] > 0);
        maxAtomsPerCell = std::max(maxAtomsPerCell, counts_[cell]);
        offsets[cell + 1] = offsets[cell] + counts_[cell];
    }

    counts_.assign(countCells, 0);
    atomsInCells.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        const std::size_t cell = cellIndices_[i];
        atomsInCells[offsets[cell] + counts_[cell]++] = i;
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
    rebuildCounter_.reset();
    metrics_.reset();
}
