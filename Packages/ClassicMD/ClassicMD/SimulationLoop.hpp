#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include "ClassicMD.hpp"

namespace ClassicMD {

/// Steps a simulation with a fixed period until asked to stop.
/// Ordinary class: composition decides whether it runs as a service.
class SimulationLoop final {
public:
    explicit SimulationLoop(ClassicMD& simulation,
                            std::chrono::milliseconds period = std::chrono::milliseconds(1000))
        : simulation_(simulation), period_(period) {}

    void run() {
        std::unique_lock lock(mutex_);
        while (!stop_) {
            lock.unlock();
            simulation_.step();
            ++steps_;
            lock.lock();
            wake_.wait_for(lock, period_, [this] { return stop_; });
        }
    }

    void requestStop() {
        {
            std::lock_guard lock(mutex_);
            stop_ = true;
        }
        wake_.notify_all();
    }

    [[nodiscard]] size_t steps() const noexcept { return steps_; }

private:
    ClassicMD& simulation_;
    std::chrono::milliseconds period_;
    std::mutex mutex_;
    std::condition_variable wake_;
    bool stop_ = false;
    std::atomic<size_t> steps_{0};
};

}
