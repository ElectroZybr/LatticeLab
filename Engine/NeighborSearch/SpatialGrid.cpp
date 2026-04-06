#include "SpatialGrid.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "../metrics/Profiler.h"

SpatialGrid::SpatialGrid(int sizeX, int sizeY, int sizeZ, int cellSize) : sizeX(0), sizeY(0), sizeZ(0), cellSize(cellSize) {
    if (sizeX < 0 || sizeY < 0 || sizeZ < 0) {
        throw std::invalid_argument("SpatialGrid::SpatialGrid: size must be > 0");
    }
    if (this->cellSize <= 0) {
        throw std::invalid_argument("SpatialGrid::SpatialGrid: cellSize must be > 0");
    }
    const int cellsX = std::max(1 + 2 * kGhostLayers, (sizeX + this->cellSize - 1) / this->cellSize + 2 * kGhostLayers);
    const int cellsY = std::max(1 + 2 * kGhostLayers, (sizeY + this->cellSize - 1) / this->cellSize + 2 * kGhostLayers);
    const int cellsZ = std::max(1 + 2 * kGhostLayers, (sizeZ + this->cellSize - 1) / this->cellSize + 2 * kGhostLayers);
    this->sizeX = cellsX;
    this->sizeY = cellsY;
    this->sizeZ = cellsZ;
    countCells = this->sizeX * this->sizeY * this->sizeZ;
    offsets.assign(countCells + 1, 0);
    rebuildNeighborOffsets();
    counts_.assign(countCells, 0);
    cellIndices_.reserve(256);
    atomsInCells.reserve(256);
}

void SpatialGrid::rebuild(std::span<const float> posX, std::span<const float> posY, std::span<const float> posZ) {
    PROFILE_SCOPE("SpatialGrid::rebuild");
    const size_t n = posX.size();

    if (n == 0) {
        atomsInCells.clear();
        std::fill(offsets.begin(), offsets.end(), 0);
        stats_.recordRebuild(0, 0, 0.0f);
        return;
    }

    cellIndices_.resize(n);
    counts_.assign(countCells, 0);

    for (size_t i = 0; i < n; ++i) {
        const size_t cell = static_cast<size_t>(index(worldToCellX(posX[i]), worldToCellY(posY[i]), worldToCellZ(posZ[i])));
        cellIndices_[i] = cell;
        ++counts_[cell];
    }

    offsets.resize(countCells + 2);
    uint32_t running = 0;
    size_t nonEmptyCellCount = 0;
    uint32_t maxAtomsPerCell = 0;
    for (size_t cell = 0; cell < countCells; ++cell) {
        const uint32_t cnt = counts_[cell];
        nonEmptyCellCount += (cnt > 0);
        maxAtomsPerCell = std::max(counts_[cell], maxAtomsPerCell);
        counts_[cell] = running;
        offsets[cell] = running;
        running += cnt;
    }
    offsets[countCells] = running;

    atomsInCells.resize(n);
    for (size_t i = 0; i < n; ++i) {
        const size_t cell = cellIndices_[i];
        atomsInCells[counts_[cell]++] = i;
    }

    const float averageAtomsPerNonEmptyCell = nonEmptyCellCount > 0 ? static_cast<float>(n) / static_cast<float>(nonEmptyCellCount) : 0.0f;
    stats_.recordRebuild(nonEmptyCellCount, maxAtomsPerCell, averageAtomsPerNonEmptyCell);
}

void SpatialGrid::resize(int newSizeX, int newSizeY, int newSizeZ, int newCellSize) {
    if (newCellSize > 0) {
        cellSize = newCellSize;
    }
    else if (newCellSize != -1) {
        throw std::invalid_argument("SpatialGrid::resize: newCellSize must be > 0");
    }

    sizeX = std::max(1 + 2 * kGhostLayers, (newSizeX + cellSize - 1) / cellSize + 2 * kGhostLayers);
    sizeY = std::max(1 + 2 * kGhostLayers, (newSizeY + cellSize - 1) / cellSize + 2 * kGhostLayers);
    sizeZ = std::max(1 + 2 * kGhostLayers, (newSizeZ + cellSize - 1) / cellSize + 2 * kGhostLayers);

    countCells = sizeX * sizeY * sizeZ;
    offsets.assign(countCells + 1, 0);
    atomsInCells.clear();
    rebuildNeighborOffsets();
    stats_.reset();
}

void SpatialGrid::rebuildNeighborOffsets() noexcept {
    /* построение массива смещений для 27 соседей */
    uint32_t k = 0;
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                neighborOffsets27_[k++] = dx + (dy + dz * sizeY) * sizeX;
            }
        }
    }
}

size_t SpatialGrid::memoryBytes() const {
    return atomsInCells.capacity() * sizeof(atomsInCells[0]) + offsets.capacity() * sizeof(offsets[0]) + sizeof(neighborOffsets27_) +
           cellIndices_.capacity() * sizeof(cellIndices_[0]) + counts_.capacity() * sizeof(counts_[0]);
}
