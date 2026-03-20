#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

class Atom;

class SpatialGrid {
public:
    int sizeX;
    int sizeY;
    int sizeZ;
    int cellSize;

    SpatialGrid(int sizeX, int sizeY, int sizeZ, int cellSize = 3);
    void resize(int newSizeX, int newSizeY, int newSizeZ, int newCellSize = -1);

    void insert(int x, int y, int z, Atom* atom);
    void erase(int x, int y, int z, Atom* atom);

    template<typename F>
    void forEachAtXY(int x, int y, F&& callback) const {
        if (x < 0 || x >= sizeX || y < 0 || y >= sizeY) return;
        for (int z = 0; z < sizeZ; z++) {
            for (Atom* atom : grid[index(x, y, z)]) {
                callback(atom);
            }
        }
    }

    const std::vector<Atom*>* at(int x, int y, int z) const;
    std::vector<Atom*>* at(int x, int y, int z);

    int worldToCellX(double x) const;
    int worldToCellY(double y) const;
    int worldToCellZ(double z) const;

private:
    std::vector<std::vector<Atom*>> grid;

    int index(int x, int y, int z) const;
    bool inBounds(int x, int y, int z) const;
    int toCell(double coord, int size) const;
};