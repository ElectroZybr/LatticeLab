#include "SpatialGrid.h"

#include <algorithm>
#include <stdexcept>

SpatialGrid::SpatialGrid(int sizeX, int sizeY, int sizeZ, int cellSize)
    : sizeX(sizeX + kBorderCells),
      sizeY(sizeY + kBorderCells),
      sizeZ(sizeZ + kBorderCells),
      cellSize(cellSize)
{
    if (sizeX < 0 || sizeY < 0 || sizeZ < 0)
        throw std::invalid_argument("SpatialGrid::SpatialGrid: invalid arguments");
}

void SpatialGrid::rebuild(std::span<const float> posX,
                          std::span<const float> posY,
                          std::span<const float> posZ) {
    const std::size_t n = posX.size();
    if (n == 0) return;
    const int totalCells = sizeX * sizeY * sizeZ;

    std::fill(cellCount.begin(), cellCount.end(), 0);
    for (std::size_t i = 0; i < n; ++i) {
        const int cx = worldToCellX(posX[i]);
        const int cy = worldToCellY(posY[i]);
        const int cz = worldToCellZ(posZ[i]);
        ++cellCount[index(cx, cy, cz)];
    }

    cellStart[0] = 0;
    for (int c = 0; c < totalCells; ++c) {
        cellStart[c + 1] = cellStart[c] + cellCount[c];
    }

    atomsSorted.resize(n);
    std::vector<int> insertPos = cellStart; // куда вставлять следующий атом
    for (std::size_t i = 0; i < n; ++i) {
        const int cx = worldToCellX(posX[i]);
        const int cy = worldToCellY(posY[i]);
        const int cz = worldToCellZ(posZ[i]);
        const int c = index(cx, cy, cz);
        atomsSorted[insertPos[c]++] = i;
    }
}

void SpatialGrid::resize(int newSizeX, int newSizeY, int newSizeZ, int newCellSize) {
    if (newSizeX < 0 || newSizeY < 0 || newSizeZ < 0)
        throw std::invalid_argument("SpatialGrid::resize: invalid arguments");

    if (newCellSize > 0) cellSize = newCellSize;
    sizeX = newSizeX + kBorderCells;
    sizeY = newSizeY + kBorderCells;
    sizeZ = newSizeZ + kBorderCells;

    const int total = sizeX * sizeY * sizeZ;
    cellCount.assign(total, 0);
    cellStart.assign(total + 1, 0);
    atomsSorted.clear();
}
