#pragma once

#include <array>
#include <cstddef>
#include "../utils/RateCounter.h"

struct NeighborListMetricsSnapshot {
    std::size_t rebuildCount = 0;
    float averageStepsBetweenRebuilds = 0.0f;
    float recentAverageStepsBetweenRebuilds = 0.0f;
    int stepsSinceLastRebuild = 0;
    float lastRebuildTimeMs = 0.0f;
    float averageRebuildTimeMs = 0.0f;
    float maxRebuildTimeMs = 0.0f;
};

class NeighborListMetrics {
public:
    static constexpr std::size_t kRecentWindow = 8;

    void reset() {
        rebuildCount_ = 0;
        rebuildIntervalsSum_ = 0;
        lastRebuildStep_ = -1;
        recentIntervals_.fill(0);
        recentCount_ = 0;
        recentCursor_ = 0;
        lastRebuildTimeMs_ = 0.0f;
        averageRebuildTimeMs_ = 0.0f;
        maxRebuildTimeMs_ = 0.0f;
    }

    void onDisable() {
        reset();
    }

    void onRebuild(int currentStep, const RateCounter& rebuildCounter) {
        onRebuild(currentStep, rebuildCounter.lastMs());
    }

    void onRebuild(int currentStep, float rebuildTimeMs) {
        if (lastRebuildStep_ >= 0 && currentStep >= lastRebuildStep_) {
            const std::size_t interval = static_cast<std::size_t>(currentStep - lastRebuildStep_);
            rebuildIntervalsSum_ += interval;

            recentIntervals_[recentCursor_] = interval;
            recentCursor_ = (recentCursor_ + 1) % kRecentWindow;
            if (recentCount_ < kRecentWindow) {
                ++recentCount_;
            }
        }

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

        lastRebuildStep_ = currentStep;
        ++rebuildCount_;
    }

    [[nodiscard]] std::size_t rebuildCount() const {
        return rebuildCount_;
    }

    [[nodiscard]] float averageStepsBetweenRebuilds() const {
        if (rebuildCount_ <= 1) {
            return 0.0f;
        }
        return static_cast<float>(rebuildIntervalsSum_) /
            static_cast<float>(rebuildCount_ - 1);
    }

    [[nodiscard]] float recentAverageStepsBetweenRebuilds() const {
        if (recentCount_ == 0) {
            return 0.0f;
        }

        std::size_t sum = 0;
        for (std::size_t index = 0; index < recentCount_; ++index) {
            sum += recentIntervals_[index];
        }
        return static_cast<float>(sum) / static_cast<float>(recentCount_);
    }

    [[nodiscard]] float lastRebuildTimeMs() const {
        return lastRebuildTimeMs_;
    }

    [[nodiscard]] float averageRebuildTimeMs() const {
        return averageRebuildTimeMs_;
    }

    [[nodiscard]] float maxRebuildTimeMs() const {
        return maxRebuildTimeMs_;
    }

    [[nodiscard]] int stepsSinceLastRebuild(int currentStep) const {
        if (lastRebuildStep_ < 0) {
            return currentStep;
        }
        return currentStep - lastRebuildStep_;
    }

    [[nodiscard]] NeighborListMetricsSnapshot snapshot(int currentStep) const {
        NeighborListMetricsSnapshot result{};
        result.rebuildCount = rebuildCount();
        result.averageStepsBetweenRebuilds = averageStepsBetweenRebuilds();
        result.recentAverageStepsBetweenRebuilds = recentAverageStepsBetweenRebuilds();
        result.stepsSinceLastRebuild = stepsSinceLastRebuild(currentStep);
        result.lastRebuildTimeMs = lastRebuildTimeMs();
        result.averageRebuildTimeMs = averageRebuildTimeMs();
        result.maxRebuildTimeMs = maxRebuildTimeMs();
        return result;
    }

private:
    std::size_t rebuildCount_ = 0;
    std::size_t rebuildIntervalsSum_ = 0;
    int lastRebuildStep_ = -1;

    std::array<std::size_t, kRecentWindow> recentIntervals_{};
    std::size_t recentCount_ = 0;
    std::size_t recentCursor_ = 0;

    float lastRebuildTimeMs_ = 0.0f;
    float averageRebuildTimeMs_ = 0.0f;
    float maxRebuildTimeMs_ = 0.0f;
};
