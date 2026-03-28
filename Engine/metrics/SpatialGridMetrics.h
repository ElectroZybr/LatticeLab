#pragma once

#include <cstddef>

struct SpatialGridMetricsSnapshot {
    std::size_t rebuildCount = 0;
    float lastRebuildTimeMs = 0.0f;
    float averageRebuildTimeMs = 0.0f;
    float maxRebuildTimeMs = 0.0f;
    std::size_t lastAtomCount = 0;
    std::size_t lastNonEmptyCellCount = 0;
    std::size_t lastMaxAtomsPerCell = 0;
    float lastAverageAtomsPerNonEmptyCell = 0.0f;
};

class SpatialGridMetrics {
public:
    void reset() {
        rebuildCount_ = 0;
        lastRebuildTimeMs_ = 0.0f;
        averageRebuildTimeMs_ = 0.0f;
        maxRebuildTimeMs_ = 0.0f;
        lastAtomCount_ = 0;
        lastNonEmptyCellCount_ = 0;
        lastMaxAtomsPerCell_ = 0;
        lastAverageAtomsPerNonEmptyCell_ = 0.0f;
    }

    void onRebuild(float rebuildTimeMs,
                   std::size_t atomCount,
                   std::size_t nonEmptyCellCount,
                   std::size_t maxAtomsPerCell) {
        lastRebuildTimeMs_ = rebuildTimeMs;
        if (rebuildTimeMs > maxRebuildTimeMs_) {
            maxRebuildTimeMs_ = rebuildTimeMs;
        }

        if (rebuildCount_ == 0) {
            averageRebuildTimeMs_ = rebuildTimeMs;
        } else {
            averageRebuildTimeMs_ += (rebuildTimeMs - averageRebuildTimeMs_) /
                static_cast<float>(rebuildCount_ + 1);
        }

        lastAtomCount_ = atomCount;
        lastNonEmptyCellCount_ = nonEmptyCellCount;
        lastMaxAtomsPerCell_ = maxAtomsPerCell;
        lastAverageAtomsPerNonEmptyCell_ = nonEmptyCellCount > 0
            ? static_cast<float>(atomCount) / static_cast<float>(nonEmptyCellCount)
            : 0.0f;

        ++rebuildCount_;
    }

    [[nodiscard]] std::size_t rebuildCount() const { return rebuildCount_; }
    [[nodiscard]] float lastRebuildTimeMs() const { return lastRebuildTimeMs_; }
    [[nodiscard]] float averageRebuildTimeMs() const { return averageRebuildTimeMs_; }
    [[nodiscard]] float maxRebuildTimeMs() const { return maxRebuildTimeMs_; }
    [[nodiscard]] std::size_t lastAtomCount() const { return lastAtomCount_; }
    [[nodiscard]] std::size_t lastNonEmptyCellCount() const { return lastNonEmptyCellCount_; }
    [[nodiscard]] std::size_t lastMaxAtomsPerCell() const { return lastMaxAtomsPerCell_; }
    [[nodiscard]] float lastAverageAtomsPerNonEmptyCell() const { return lastAverageAtomsPerNonEmptyCell_; }

    [[nodiscard]] SpatialGridMetricsSnapshot snapshot() const {
        SpatialGridMetricsSnapshot result{};
        result.rebuildCount = rebuildCount();
        result.lastRebuildTimeMs = lastRebuildTimeMs();
        result.averageRebuildTimeMs = averageRebuildTimeMs();
        result.maxRebuildTimeMs = maxRebuildTimeMs();
        result.lastAtomCount = lastAtomCount();
        result.lastNonEmptyCellCount = lastNonEmptyCellCount();
        result.lastMaxAtomsPerCell = lastMaxAtomsPerCell();
        result.lastAverageAtomsPerNonEmptyCell = lastAverageAtomsPerNonEmptyCell();
        return result;
    }

private:
    std::size_t rebuildCount_ = 0;
    float lastRebuildTimeMs_ = 0.0f;
    float averageRebuildTimeMs_ = 0.0f;
    float maxRebuildTimeMs_ = 0.0f;

    std::size_t lastAtomCount_ = 0;
    std::size_t lastNonEmptyCellCount_ = 0;
    std::size_t lastMaxAtomsPerCell_ = 0;
    float lastAverageAtomsPerNonEmptyCell_ = 0.0f;
};
