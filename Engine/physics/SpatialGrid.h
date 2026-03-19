#pragma once
#include <unordered_set>
#include <vector>
#include <cmath>

class Atom;

class SpatialGrid {
private:
    std::vector<std::unordered_set<Atom*>> grid;
public:
    int sizeX;
    int sizeY;
    int sizeZ;
    int cellSize;

    SpatialGrid(int sizeX, int sizeY, int sizeZ, int cellSize = 3);
    void resize(int newSizeX, int newSizeY, int newSizeZ, int newCellSize = -1);

    void insert(int x, int y, int z, Atom* val) {
        if (auto cell = at(x, y, z)) {
            cell->insert(val);
        }
    }

    void erase(int x, int y,int z, Atom* val) {
        if (auto cell = at(x, y, z)) {
            cell->erase(val);
        }
    }

    std::unordered_set<Atom*>* at(int x, int y) {
        static std::unordered_set<Atom*> result;
        result.clear();
        for (int z = 0; z < sizeZ; z++) {
            if (x >= 0 && x < sizeX && y >= 0 && y < sizeY) {
                auto& cell = grid[z * sizeY * sizeX + y * sizeX + x];
                result.insert(cell.begin(), cell.end());
            }
        }
        return result.empty() ? nullptr : &result;
    }
    const std::unordered_set<Atom*>* at(int x, int y) const {
        return this->at(x, y);
    }

    std::unordered_set<Atom*>* at(int x, int y, int z) {
        if (x >= 0 && x < sizeX && y >= 0 && y < sizeY && z >= 0 && z < sizeZ)
            return &grid[z * sizeY * sizeX + y * sizeX + x];
        return nullptr;
    }
    const std::unordered_set<Atom*>* at(int x, int y, int z) const {
        return this->at(x, y, z);
    }

    int worldToCellX(double x) const {
        if (x < 0.0) return -1;
        return x / cellSize;
    }

    int worldToCellY(double y) const {
        if (y < 0.0) return -1;
        return y / cellSize;
    }

    int worldToCellZ(double z) const {
        if (z < 0.0) return -1;
        return z / cellSize;
    }
};
