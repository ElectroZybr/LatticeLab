#include "SpatialGrid.h"

#include <algorithm>
#include <stdexcept>

SpatialGrid::SpatialGrid(int sizeX, int sizeY, int sizeZ, int cellSize)
    : sizeX(sizeX),
      sizeY(sizeY),
      sizeZ(sizeZ),
      cellSize(cellSize),
      indexGrid(sizeX * sizeY * sizeZ) {
    if (sizeX < 0 || sizeY < 0 || sizeZ < 0)
        throw std::invalid_argument("SpatialGrid::SpatialGrid: invalid arguments");
}

void SpatialGrid::resize(int newSizeX, int newSizeY, int newSizeZ, int newCellSize) {
    if (newSizeX < 0 || newSizeY < 0 || newSizeZ < 0)
        throw std::invalid_argument("SpatialGrid::resize: invalid arguments");

    if (newCellSize > 0) cellSize = newCellSize;
    sizeX = newSizeX;
    sizeY = newSizeY;
    sizeZ = newSizeZ;
    indexGrid.assign(sizeX * sizeY * sizeZ, {});
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
