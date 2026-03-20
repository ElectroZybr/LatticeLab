#pragma once
#include <vector>
#include <cmath>

#include <Engine/math/Vec3D.h>

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

    const std::vector<Atom*>* at(int x, int y, int z) const;
    std::vector<Atom*>* at(int x, int y, int z);

    int worldToCellX(double x) const;
    int worldToCellY(double y) const;
    int worldToCellZ(double z) const;

    template<typename F>
    void forEachAtXY(int x, int y, F&& callback) const {
        if (x < 0 || x >= sizeX || y < 0 || y >= sizeY) return;
        for (int z = 0; z < sizeZ; z++) {
            for (Atom* atom : grid[index(x, y, z)]) {
                callback(atom);
            }
        }
    }

    template<typename F>
    void forEachNeighbor(const Vec3D& coords, F&& callback) const {
        const int cx = worldToCellX(coords.x);
        const int cy = worldToCellY(coords.y);
        const int cz = worldToCellZ(coords.z);
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                for (int k = -1; k <= 1; ++k) {
                    if (const auto* cell = at(cx + i, cy + j, cz + k)) {
                        for (Atom* atom : *cell) {
                            callback(atom);
                        }
                    }
                }
            }
        }
    }
private:
    std::vector<std::vector<Atom*>> grid;

    int index(int x, int y, int z) const;
    bool inBounds(int x, int y, int z) const;
    int toCell(double coord, int size) const;
};