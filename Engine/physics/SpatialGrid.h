#pragma once

#include <cstddef>
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
    [[nodiscard]] std::span<const size_t> atomsInCell(int x, int y, int z) const noexcept {
        const size_t idx = static_cast<size_t>(index(x, y, z));
        const size_t begin = offsets[idx];
        return std::span<const size_t>(atomsInCells.data() + begin, offsets[idx + 1] - begin);
    }

    int worldToCellX(float x) const { return toCell(x, sizeX); }
    int worldToCellY(float y) const { return toCell(y, sizeY); }
    int worldToCellZ(float z) const { return toCell(z, sizeZ); }

    [[nodiscard]] int countAtomsInCell(int cx, int cy, int cz) const {
        const size_t idx = static_cast<size_t>(index(cx, cy, cz));
        return static_cast<int>(offsets[idx + 1] - offsets[idx]);
    }

private:
    // CSR хранение данных
    std::vector<size_t> offsets;      // массив оффсетов (каждый оффсет - начало новой ячейки)
    std::vector<size_t> atomsInCells; // атомы подряд сгруппированные по ячейкам
    
    // рабочие буферы rebuild — переиспользуются между вызовами
    std::vector<size_t> cellIndices_;
    std::vector<size_t> counts_;

    static constexpr int kBorderCells = 2; // запас + 1 клетка с каждой стороны от бокса

    RateCounter rebuildCounter_;
    SpatialGridMetrics metrics_;

    [[nodiscard]] int index(int x, int y, int z) const noexcept {
        return z * sizeY * sizeX + y * sizeX + x;
    }

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
