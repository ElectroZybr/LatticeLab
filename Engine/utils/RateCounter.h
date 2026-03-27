#pragma once

#include <cmath>

#include "Timer.h"

class RateCounter {
private:
    Timer timer_;
    double accumulatedMs_ = 0.0;
    int stepsThisTick_ = 0;

    double totalMs_ = 0.0;
    int totalSteps_ = 0;
    float lastStepMs_ = 0.0f;
    float maxStepMs_ = 0.0f;

    bool hasRateSample = false;
    
public:
    float stepsPerSecond = 0.0f;
    
    void startStep() { timer_.start(); }

    void finishStep() {
        timer_.stop();
        const float elapsedMs = static_cast<float>(timer_.elapsedMilliseconds());
        lastStepMs_ = elapsedMs;
        if (elapsedMs > maxStepMs_) {
            maxStepMs_ = elapsedMs;
        }

        accumulatedMs_ += elapsedMs;
        ++stepsThisTick_;

        totalMs_ += elapsedMs;
        ++totalSteps_;
    }

    float avgMs() const {
        return stepsThisTick_ > 0 ? static_cast<float>(accumulatedMs_ / stepsThisTick_) : 0.0f;
    }

    float totalAvgMs() const {
        return totalSteps_ > 0 ? static_cast<float>(totalMs_ / totalSteps_) : 0.0f;
    }

    float lastMs() const {
        return lastStepMs_;
    }

    float maxMs() const {
        return maxStepMs_;
    }

    int totalSteps() const {
        return totalSteps_;
    }

    void flush(double elapsedSeconds) {
        if (elapsedSeconds > 0.0) {
            const float instantRate = static_cast<float>(stepsThisTick_ / elapsedSeconds);
            const float alpha = static_cast<float>(1.0 - std::exp(-elapsedSeconds / 0.5));
            if (!hasRateSample) {
                stepsPerSecond = instantRate;
                hasRateSample = true;
            } else {
                stepsPerSecond += alpha * (instantRate - stepsPerSecond);
            }
        }
        accumulatedMs_ = 0.0;
        stepsThisTick_ = 0;
    }

    void reset() {
        accumulatedMs_ = 0.0;
        stepsThisTick_ = 0;
        totalMs_ = 0.0;
        totalSteps_ = 0;
        lastStepMs_ = 0.0f;
        maxStepMs_ = 0.0f;
        stepsPerSecond = 0.0f;
        hasRateSample = false;
    }
};
