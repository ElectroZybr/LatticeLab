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
    int cellSize;

    SpatialGrid(int sizeX, int sizeY, int cellSize = 3);
    void resize(int newSizeX, int newSizeY, int newCellSize = -1);

    void insert(int x, int y, Atom* val) {
        if (auto cell = at(x, y)) {
            cell->insert(val);
        }
    }

    void erase(int x, int y, Atom* val) {
        if (auto cell = at(x, y)) {
            cell->erase(val);
        }
    }

    std::unordered_set<Atom*>* at(int x, int y) {
        if (x >= 0 && x < sizeX && y >= 0 && y < sizeY)
            return &grid[y * sizeX + x];
        return nullptr;
    }
    const std::unordered_set<Atom*>* at(int x, int y) const {
        if (x >= 0 && x < sizeX && y >= 0 && y < sizeY)
            return &grid[y * sizeX + x];
        return nullptr;
    }


    int worldToCellX(double x) const {
        if (x < 0.0) return -1;
        return static_cast<int>(x / cellSize);
    }

    int worldToCellY(double y) const {
        if (y < 0.0) return -1;
        return static_cast<int>(y / cellSize);
    }
};
