#include "SpatialGrid.h"

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
    const std::size_t n = posX.size();
    if (n != posY.size() || n != posZ.size()) {
        throw std::invalid_argument("SpatialGrid::rebuild: inconsistent coordinate span sizes");
    }

    std::vector<std::size_t> counts(countCells, 0);
    for (std::size_t i = 0; i < n; ++i) {
        const int cx = worldToCellX(posX[i]);
        const int cy = worldToCellY(posY[i]);
        const int cz = worldToCellZ(posZ[i]);
        ++counts[static_cast<std::size_t>(index(cx, cy, cz))];
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
}
