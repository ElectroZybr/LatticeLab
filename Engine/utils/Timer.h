#pragma once

#include <chrono>
#include <cstddef>

class Timer {
private:
    using Clock = std::chrono::high_resolution_clock;

    Clock::time_point startTime_{};
    Clock::time_point endTime_{};
    bool isRunning_ = false;

public:
    inline void start() {
        startTime_ = Clock::now();
        isRunning_ = true;
    }

    inline void stop() {
        endTime_ = Clock::now();
        isRunning_ = false;
    }

    [[nodiscard]] inline double elapsedMilliseconds() const {
        return elapsedSeconds() * 1000.0;
    }

    [[nodiscard]] inline double elapsedSeconds() const {
        const Clock::time_point now = isRunning_ ? Clock::now() : endTime_;
        return std::chrono::duration<double>(now - startTime_).count();
    }
};