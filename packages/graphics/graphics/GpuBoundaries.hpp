#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

#include <boundaries/Program.hpp>
#include <boundaries/Space.hpp>
#include <boundarieswgpu/GpuGeometry.hpp>
#include <particledynamics/ParticleSystemAPI.hpp>
#include <wgpu/Device.hpp>

/// The boundaries of a simulation for drawing: its space and its geometry program on the
/// device. A copy is taken by a command of the simulation's queue when their version
/// changes and applied by update() on the host; set() fills them directly.
class GpuBoundaries final {
public:
    explicit GpuBoundaries(Wgpu::Device& device) : device_(device) {}
    GpuBoundaries(Wgpu::Device& device, ParticleDynamics::ParticleSystemAPI& system);
    ~GpuBoundaries();

    void set(const Boundaries::Space& space, Boundaries::Program program);
    void update();

    void detach() noexcept { attached_ = false; }
    bool idle() const noexcept { return !pending_; }

    /// Null until boundaries arrive (or when the simulation has none).
    const Boundaries::Wgpu::GpuGeometry* geometry() const noexcept { return geometry_.get(); }
    const Boundaries::Space* space() const noexcept { return space_ ? &*space_ : nullptr; }
    uint64_t version() const noexcept { return version_; }

private:
    struct Copy {
        Boundaries::Space space;
        Boundaries::Program program;
        uint64_t version;
    };

    Wgpu::Device& device_;
    ParticleDynamics::ParticleSystemAPI* system_ = nullptr;
    std::unique_ptr<Boundaries::Wgpu::GpuGeometry> geometry_;
    std::optional<Boundaries::Space> space_;
    uint64_t version_ = 0;
    uint64_t copied_ = 0;
    std::mutex mutex_;
    std::optional<Copy> arrived_;
    std::atomic<bool> pending_{false};
    std::atomic<bool> attached_{false};
};
