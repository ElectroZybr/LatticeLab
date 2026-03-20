#include "SpatialGrid.h"
#include <stdexcept>

SpatialGrid::SpatialGrid(int sizeX, int sizeY, int sizeZ, int cellSize)
    : sizeX(sizeX),
      sizeY(sizeY),
      sizeZ(sizeZ),
      cellSize(cellSize),
      grid(sizeX * sizeY * sizeZ) {
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
    grid.assign(sizeX * sizeY * sizeZ, {});
}

void SpatialGrid::insert(int x, int y, int z, Atom* atom) {
    if (auto* cell = at(x, y, z))
        cell->emplace_back(atom);
}

void SpatialGrid::erase(int x, int y, int z, Atom* atom) {
    if (auto* cell = at(x, y, z)) {
        auto it = std::find(cell->begin(), cell->end(), atom);
        if (it != cell->end()) {
            *it = cell->back();
            cell->pop_back();
        }
    }
}

const std::vector<Atom*>* SpatialGrid::at(int x, int y, int z) const {
    if (inBounds(x, y, z))
        return &grid[index(x, y, z)];
    return nullptr;
}

std::vector<Atom*>* SpatialGrid::at(int x, int y, int z) {
    if (inBounds(x, y, z))
        return &grid[index(x, y, z)];
    return nullptr;
}

int SpatialGrid::worldToCellX(double x) const { return toCell(x, sizeX); }
int SpatialGrid::worldToCellY(double y) const { return toCell(y, sizeY); }
int SpatialGrid::worldToCellZ(double z) const { return toCell(z, sizeZ); }

int SpatialGrid::index(int x, int y, int z) const {
    return z * sizeY * sizeX + y * sizeX + x;
}

bool SpatialGrid::inBounds(int x, int y, int z) const {
    return x >= 0 && x < sizeX
        && y >= 0 && y < sizeY
        && z >= 0 && z < sizeZ;
}

int SpatialGrid::toCell(double coord, int size) const {
    if (coord < 0.0) return -1;
    int c = static_cast<int>(coord / cellSize);
    return c < size ? c : -1;
}