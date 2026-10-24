#pragma once

#include <atomic>
#include <chrono>

#include <ParticleDynamics/ParticleAPI.hpp>
#include <ParticleDynamics/ParticleSystemAPI.hpp>
#include <glm/vec3.hpp>

namespace ClassicMD {

/// Classical MD simulation. run() steps it with a fixed period until requestStop();
/// composition decides whether it runs as a service.
///
/// The thread running run() owns the state. Everything else changes it through the
/// command queue it serves: extra steps, parameters, copies for rendering run between
/// steps. Without run() commands execute at once on their callers, one at a time.
class ClassicMD final : public ParticleDynamics::ParticleSystemAPI {
public:
    ClassicMD(ParticleDynamics::ParticleStorage& atoms,
              ParticleDynamics::IntegratorAPI& integrator,
              Tasks::Queue& commands,
              std::chrono::milliseconds period = std::chrono::milliseconds(1000))
        : atoms_(atoms), integrator_(integrator), commands_(commands), period_(period) {
        atoms.add({0, 0, 0}, {1, 10, 0});
    }

    /// One step, on the owner thread (from run() or a command).
    void step() {
        integrator_.step();
        ++steps_;
    }

    void run() {
        commands_.beginServing();
        auto next = std::chrono::steady_clock::now();
        while (!stop_) {
            commands_.runUntil(next, [this] { return stop_.load(); });
            if (stop_)
                break;
            step();
            next = std::chrono::steady_clock::now() + period_;
        }
        commands_.endServing();
    }

    void requestStop() {
        stop_ = true;
        commands_.wake();
    }

    const ParticleDynamics::ParticleStorage& particles() const override { return atoms_; }
    Tasks::Queue& commands() override { return commands_; }

    [[nodiscard]] size_t steps() const noexcept { return steps_; }

    glm::vec3 cellSize{};

private:
    ParticleDynamics::ParticleStorage& atoms_;
    ParticleDynamics::IntegratorAPI& integrator_;
    Tasks::Queue& commands_;
    std::chrono::milliseconds period_;
    std::atomic<bool> stop_{false};
    std::atomic<size_t> steps_{0};
};

}
