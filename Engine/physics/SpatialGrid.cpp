#include "SpatialGrid.h"

#include <cmath>
#include <stdexcept>

SpatialGrid::SpatialGrid(int sizeX, int sizeY, int sizeZ, int cellSize)
    : sizeX(sizeX),
      sizeY(sizeY),
      sizeZ(sizeZ),
      cellSize(cellSize),
      grid(sizeX * sizeY * sizeZ) {
}

void SpatialGrid::resize(int newSizeX, int newSizeY, int newSizeZ, int newCellSize) {
    if (newSizeX < 0 || newSizeY < 0 || newSizeZ < 0)
        throw std::invalid_argument("SpatialGrid::resize: invalid arguments");

    if (newCellSize > 0) cellSize = newCellSize;
    sizeX = newSizeX;
    sizeY = newSizeY;
    sizeZ = newSizeY;
    grid.assign(sizeX * sizeY * sizeZ, {});
}