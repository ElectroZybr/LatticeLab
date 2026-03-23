#pragma once
#include <vector>
#include <cmath>
#include <cstddef>

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
    void insertIndex(int x, int y, int z, std::size_t atomIndex);
    void eraseIndex(int x, int y, int z, std::size_t atomIndex);

    [[nodiscard]] const std::vector<Atom*>* at(int x, int y, int z) const {
        return inBounds(x, y, z) ? &grid[index(x, y, z)] : nullptr;
    }
    [[nodiscard]] std::vector<Atom*>* at(int x, int y, int z) {
        return inBounds(x, y, z) ? &grid[index(x, y, z)] : nullptr;
    }
    [[nodiscard]] const std::vector<std::size_t>* atIndex(int x, int y, int z) const {
        return inBounds(x, y, z) ? &indexGrid[index(x, y, z)] : nullptr;
    }
    [[nodiscard]] std::vector<std::size_t>* atIndex(int x, int y, int z) {
        return inBounds(x, y, z) ? &indexGrid[index(x, y, z)] : nullptr;
    }

    int worldToCellX(double x) const  { return toCell(x, sizeX); };
    int worldToCellY(double y) const  { return toCell(y, sizeY); };
    int worldToCellZ(double z) const  { return toCell(z, sizeZ); };

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

        const int x0 = std::max(cx - 1, 0),  x1 = std::min(cx + 1, sizeX - 1);
        const int y0 = std::max(cy - 1, 0),  y1 = std::min(cy + 1, sizeY - 1);
        const int z0 = std::max(cz - 1, 0),  z1 = std::min(cz + 1, sizeZ - 1);

        for (int iz = z0; iz <= z1; ++iz) {
            for (int iy = y0; iy <= y1; ++iy) {
                for (int ix = x0; ix <= x1; ++ix) {
                    for (Atom* atom : grid[index(ix, iy, iz)]) {
                        callback(atom);
                    }
                }
            }
        }
    }

    template<typename F>
    void forEachNeighborIndex(const Vec3D& coords, F&& callback) const {
        const int cx = worldToCellX(coords.x);
        const int cy = worldToCellY(coords.y);
        const int cz = worldToCellZ(coords.z);

        const int x0 = std::max(cx - 1, 0),  x1 = std::min(cx + 1, sizeX - 1);
        const int y0 = std::max(cy - 1, 0),  y1 = std::min(cy + 1, sizeY - 1);
        const int z0 = std::max(cz - 1, 0),  z1 = std::min(cz + 1, sizeZ - 1);

        for (int ix = x0; ix <= x1; ++ix) {
            for (int iy = y0; iy <= y1; ++iy) {
                for (int iz = z0; iz <= z1; ++iz) {
                    for (std::size_t atomIndex : indexGrid[index(ix, iy, iz)]) {
                        callback(atomIndex);
                    }
                }
            }
        }
    }
private:
    std::vector<std::vector<Atom*>> grid;
    std::vector<std::vector<std::size_t>> indexGrid;

    [[nodiscard]] int index(int x, int y, int z) const noexcept {
        return z * sizeY * sizeX + y * sizeX + x;
    }
    [[nodiscard]] bool inBounds(int x, int y, int z) const noexcept {
        return x >= 0 && x < sizeX
            && y >= 0 && y < sizeY
            && z >= 0 && z < sizeZ;
    }
    [[nodiscard]] int toCell(double coord, int size) const {
        if (coord < 0.0) return -1;
        int c = static_cast<int>(coord / cellSize);
        return c < size ? c : -1;
    }
};
