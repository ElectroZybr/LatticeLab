#pragma once
#include <vector>
#include <cmath>
#include <cstddef>

#include <Engine/math/Vec3f.h>

class SpatialGrid {
public:
    int sizeX;
    int sizeY;
    int sizeZ;
    int cellSize;

    SpatialGrid(int sizeX, int sizeY, int sizeZ, int cellSize = 6);
    void resize(int newSizeX, int newSizeY, int newSizeZ, int newCellSize = -1);

    void insertIndex(int x, int y, int z, std::size_t atomIndex);
    void eraseIndex(int x, int y, int z, std::size_t atomIndex);

    // c проверкой на границы
    [[nodiscard]] const std::vector<std::size_t>* atIndex(int x, int y, int z) const {
        return inBounds(x, y, z) ? &indexGrid[index(x, y, z)] : nullptr;
    }
    [[nodiscard]] std::vector<std::size_t>* atIndex(int x, int y, int z) {
        return inBounds(x, y, z) ? &indexGrid[index(x, y, z)] : nullptr;
    }
    
    // (warning) без проверок на границы
    [[nodiscard]] const std::vector<std::size_t>& atIndexUnchecked(int x, int y, int z) const noexcept {
        return indexGrid[index(x, y, z)];
    }
    [[nodiscard]] std::vector<std::size_t>& atIndexUnchecked(int x, int y, int z) noexcept {
        return indexGrid[index(x, y, z)];
    }

    int worldToCellX(float x) const  { return toCell(x, sizeX); };
    int worldToCellY(float y) const  { return toCell(y, sizeY); };
    int worldToCellZ(float z) const  { return toCell(z, sizeZ); };
private:
    std::vector<std::vector<std::size_t>> indexGrid;

    [[nodiscard]] int index(int x, int y, int z) const noexcept {
        return z * sizeY * sizeX + y * sizeX + x;
    }
    [[nodiscard]] bool inBounds(int x, int y, int z) const noexcept {
        return x >= 0 && x < sizeX
            && y >= 0 && y < sizeY
            && z >= 0 && z < sizeZ;
    }
    [[nodiscard]] int toCell(float coord, int size) const {
        if (coord < 0.0f) return -1;
        int c = static_cast<int>(coord / cellSize);
        return c < size ? c : -1;
    }
};

