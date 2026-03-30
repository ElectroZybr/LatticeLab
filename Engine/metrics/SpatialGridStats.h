#pragma once

#include <cstddef>

class SpatialGridStats {
public:
    void reset() {
        rebuildCount_ = 0;
        lastNonEmptyCellCount_ = 0;
        lastMaxAtomsPerCell_ = 0;
        lastAverageAtomsPerNonEmptyCell_ = 0.0f;
    }

    void recordRebuild(std::size_t nonEmptyCellCount, std::size_t maxAtomsPerCell, float averageAtomsPerNonEmptyCell) {
        ++rebuildCount_;
        lastNonEmptyCellCount_ = nonEmptyCellCount;
        lastMaxAtomsPerCell_ = maxAtomsPerCell;
        lastAverageAtomsPerNonEmptyCell_ = averageAtomsPerNonEmptyCell;
    }

    [[nodiscard]] std::size_t rebuildCount() const noexcept { return rebuildCount_; }
    [[nodiscard]] std::size_t lastNonEmptyCellCount() const noexcept { return lastNonEmptyCellCount_; }
    [[nodiscard]] std::size_t lastMaxAtomsPerCell() const noexcept { return lastMaxAtomsPerCell_; }
    [[nodiscard]] float lastAverageAtomsPerNonEmptyCell() const noexcept { return lastAverageAtomsPerNonEmptyCell_; }

private:
    std::size_t rebuildCount_ = 0;
    std::size_t lastNonEmptyCellCount_ = 0;
    std::size_t lastMaxAtomsPerCell_ = 0;
    float lastAverageAtomsPerNonEmptyCell_ = 0.0f;
};
