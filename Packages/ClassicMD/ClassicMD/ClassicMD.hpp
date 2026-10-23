#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <ParticleDynamics/ParticleAPI.hpp>
#include <ParticleDynamics/ParticleStorage.hpp>
#include <glm/vec3.hpp>

namespace ClassicMD {

/// Classical MD simulation. run() steps it with a fixed period until requestStop();
/// composition decides whether it runs as a service.
class ClassicMD final {
public:
    ClassicMD(ParticleDynamics::ParticleStorage& atoms,
              ParticleDynamics::IntegratorAPI& integrator,
              std::chrono::milliseconds period = std::chrono::milliseconds(1000))
        : integrator_(integrator), period_(period) {
        atoms.add({0, 0, 0}, {1, 10, 0});
    }

    void step() {
        integrator_.step();
        ++steps_;
    }

    void run() {
        std::unique_lock lock(mutex_);
        while (!stop_) {
            lock.unlock();
            step();
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

    glm::vec3 cellSize{};

private:
    ParticleDynamics::IntegratorAPI& integrator_;
    std::chrono::milliseconds period_;
    std::mutex mutex_;
    std::condition_variable wake_;
    bool stop_ = false;
    std::atomic<size_t> steps_{0};
};

}
