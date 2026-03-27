#include "SpatialGrid.h"

#include <algorithm>
#include <stdexcept>

SpatialGrid::SpatialGrid(int sizeX, int sizeY, int sizeZ, int cellSize)
    : sizeX(sizeX + kBorderCells),
      sizeY(sizeY + kBorderCells),
      sizeZ(sizeZ + kBorderCells),
      cellSize(cellSize),
      indexGrid((sizeX + kBorderCells) * (sizeY + kBorderCells) * (sizeZ + kBorderCells)) {
    if (sizeX < 0 || sizeY < 0 || sizeZ < 0)
        throw std::invalid_argument("SpatialGrid::SpatialGrid: invalid arguments");
}

void SpatialGrid::resize(int newSizeX, int newSizeY, int newSizeZ, int newCellSize) {
    if (newSizeX < 0 || newSizeY < 0 || newSizeZ < 0)
        throw std::invalid_argument("SpatialGrid::resize: invalid arguments");

    if (newCellSize > 0) cellSize = newCellSize;
    sizeX = newSizeX + kBorderCells;
    sizeY = newSizeY + kBorderCells;
    sizeZ = newSizeZ + kBorderCells;
    indexGrid.assign(sizeX * sizeY * sizeZ, {});
}

void SpatialGrid::clear() noexcept {
    for (auto& cell : indexGrid) {
        cell.clear();
    }
}

void SpatialGrid::insertIndex(int x, int y, int z, std::size_t atomIndex) {
    if (auto* cell = atIndex(x, y, z))
        cell->emplace_back(atomIndex);
}

void SpatialGrid::eraseIndex(int x, int y, int z, std::size_t atomIndex) {
    if (auto* cell = atIndex(x, y, z)) {
        auto it = std::find(cell->begin(), cell->end(), atomIndex);
        if (it != cell->end()) {
            *it = cell->back();
            cell->pop_back();
        }
    }
}
