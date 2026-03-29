#pragma once

#include <cstddef>
#include <array>
#include <span>
#include <vector>

#include <Engine/math/Vec3f.h>

#include "../metrics/SpatialGridMetrics.h"
#include "../utils/RateCounter.h"

class SpatialGrid {
public:
    int sizeX;
    int sizeY;
    int sizeZ;
    int cellSize;
    int countCells;

    SpatialGrid(int sizeX, int sizeY, int sizeZ, int cellSize = 6);

    void rebuild(std::span<const float> posX, std::span<const float> posY, std::span<const float> posZ);
    void resize(int newSizeX, int newSizeY, int newSizeZ, int newCellSize = -1);

    [[nodiscard]] const SpatialGridMetrics& metrics() const noexcept { return metrics_; }
    [[nodiscard]] const RateCounter& rebuildCounter() const noexcept { return rebuildCounter_; }

    // (warning) нет проверки выхода за границы
    [[nodiscard]] std::span<const std::size_t> atomsInCell(int x, int y, int z) const noexcept {
        const std::size_t idx = static_cast<std::size_t>(index(x, y, z));
        const std::size_t begin = offsets[idx];
        return std::span<const std::size_t>(atomsInCells.data() + begin, offsets[idx + 1] - begin);
    }

    // (warning) нет проверки выхода за границы
    [[nodiscard]] std::span<const std::size_t> atomsInCellByLinearIndex(int linearIndex) const noexcept {
        const std::size_t idx = static_cast<std::size_t>(linearIndex);
        const std::size_t begin = offsets[idx];
        return std::span<const std::size_t>(atomsInCells.data() + begin, offsets[idx + 1] - begin);
    }

    int worldToCellX(float x) const { return toCell(x, sizeX); }
    int worldToCellY(float y) const { return toCell(y, sizeY); }
    int worldToCellZ(float z) const { return toCell(z, sizeZ); }

    [[nodiscard]] int countAtomsInCell(int cx, int cy, int cz) const {
        const std::size_t idx = static_cast<std::size_t>(index(cx, cy, cz));
        return static_cast<int>(offsets[idx + 1] - offsets[idx]);
    }

    [[nodiscard]] int linearIndex(int x, int y, int z) const noexcept { return index(x, y, z); }
    [[nodiscard]] const std::array<int, 27>& neighborOffsets27() const noexcept { return neighborOffsets27_; }

private:
    // CSR хранение данных
    std::vector<std::size_t> offsets;      // массив оффсетов (каждый оффсет - начало новой ячейки)
    std::vector<std::size_t> atomsInCells; // атомы подряд сгруппированные по ячейкам
    std::array<int, 27> neighborOffsets27_{};

    static constexpr int kBorderCells = 2; // запас + 1 клетка с каждой стороны от бокса

    RateCounter rebuildCounter_;
    SpatialGridMetrics metrics_;

    [[nodiscard]] int index(int x, int y, int z) const noexcept {
        return z * sizeY * sizeX + y * sizeX + x;
    }

    void rebuildNeighborOffsets() noexcept;

    [[nodiscard]] bool inBounds(int x, int y, int z) const noexcept {
        return x >= 0 && x < sizeX && y >= 0 && y < sizeY && z >= 0 && z < sizeZ;
    }

    [[nodiscard]] int toCell(float coord, int size) const {
        if (size <= 2) {
            return 1;
        }

        const int maxInterior = size - 2;
        int c = static_cast<int>(coord / cellSize) + 1;
        if (c < 1) c = 1;
        if (c > maxInterior) c = maxInterior;
        return c;
    }
};
