#include "SpatialGrid.h"

#include <cmath>
#include <stdexcept>

SpatialGrid::SpatialGrid(int sizeX, int sizeY, int cellSize)
    : sizeX(sizeX),
      sizeY(sizeY),
      cellSize(cellSize),
      grid(sizeX * sizeY) {
}

void SpatialGrid::resize(int newSizeX, int newSizeY, int newCellSize) {
    if (newSizeX < 0 || newSizeY < 0)
        throw std::invalid_argument("SpatialGrid::resize: invalid arguments");

    if (newCellSize > 0) cellSize = newCellSize;
    sizeX = newSizeX;
    sizeY = newSizeY;
    grid.assign(sizeX * sizeY, {});
}