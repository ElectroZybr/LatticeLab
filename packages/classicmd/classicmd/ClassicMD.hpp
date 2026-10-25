#pragma once

#include <atomic>
#include <chrono>

#include <Lattice/kernel/PipelineAPI.hpp>
#include <particledynamics/ParticleSystemAPI.hpp>
#include <glm/vec3.hpp>

#include "Boundary.hpp"

namespace ClassicMD {

/// Classical MD simulation. A step runs the step pipeline (integrator halves, constraints,
/// forces — nodes placed by composition). run() steps it with a fixed period until
/// requestStop(); composition decides whether it runs as a service.
///
/// The thread running run() owns the state. Everything else changes it through the
/// command queue it serves: extra steps, parameters, copies for rendering run between
/// steps. Without run() commands execute at once on their callers, one at a time.
class ClassicMD final : public ParticleDynamics::ParticleSystemAPI {
public:
    ClassicMD(ParticleDynamics::ParticleStorage& atoms, Lattice::PipelineAPI& step, Tasks::Queue& commands,
              std::chrono::milliseconds period = std::chrono::milliseconds(16))
        : atoms_(atoms), step_(step), commands_(commands), period_(period) {}

    ClassicMD(ParticleDynamics::ParticleStorage& atoms, Lattice::PipelineAPI& step, Tasks::Queue& commands,
              const Boundary& boundary, std::chrono::milliseconds period = std::chrono::milliseconds(16))
        : atoms_(atoms), step_(step), commands_(commands), boundary_(&boundary), period_(period) {}

    /// One step, on the owner thread (from run() or a command).
    void step() {
        step_.run();
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
    ParticleDynamics::BoundaryView boundaries() const override {
        return boundary_ ? boundary_->view() : ParticleDynamics::BoundaryView{};
    }
    Tasks::Queue& commands() override { return commands_; }

    [[nodiscard]] size_t steps() const noexcept { return steps_; }

    glm::vec3 cellSize{};

private:
    ParticleDynamics::ParticleStorage& atoms_;
    Lattice::PipelineAPI& step_;
    Tasks::Queue& commands_;
    const Boundary* boundary_ = nullptr;
    std::chrono::milliseconds period_;
    std::atomic<bool> stop_{false};
    std::atomic<size_t> steps_{0};
};

}
