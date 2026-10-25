#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <boundaries/Laws.hpp>
#include <boundaries/ParticleLayout.hpp>
#include <wgpu/CommandList.hpp>
#include <wgpu/GpuSoA.hpp>
#include <wgpu/Timer.hpp>

#include "Codegen.hpp"
#include <wgpu/Resources.hpp>

#include "GpuGeometry.hpp"

namespace Boundaries::Wgpu {

/**
 * Walls on the GPU: the same materials, laws and stages as Boundaries::Cpu::Walls,
 * one invocation per particle over the columns of a GpuSoA (by the layout).
 *
 * The stages record compute passes into a command list and do not submit: a step is
 * one batch. Each stage keeps its own parameters, so record each stage at most once
 * per submitted list.
 */
class GpuWalls final {
public:
    GpuWalls(const GpuGeometry& geometry, ParticleLayout layout, std::vector<Material> materials = {Laws::Hard::make()});
    ~GpuWalls();

    /// Force stage: add wall forces (Soft) to the force columns of rows [0, rows).
    /// With a timer, the GPU time of the pass is measured as "forces".
    void addForces(::Wgpu::CommandList& commands, ::Wgpu::GpuSoA& particles, size_t rows,
                   ::Wgpu::Timer* timer = nullptr);
    /// Constrain stage: move rows [0, rows) out of walls and reflect their velocities (Hard);
    /// measured as "constrain".
    void constrain(::Wgpu::CommandList& commands, ::Wgpu::GpuSoA& particles, size_t rows,
                   ::Wgpu::Timer* timer = nullptr);

    static constexpr uint32_t WorkgroupSize = 64;

private:
    struct Stage;
    void record(::Wgpu::CommandList& commands, ::Wgpu::GpuSoA& particles, size_t rows, Stage& stage,
                ::Wgpu::Timer* timer);

    uint64_t errorsBefore_;   // first: counted before any GPU object is created
    const GpuGeometry& geometry_;
    ParticleLayout layout_;
    std::vector<Material> materials_;
    float forceRange_ = 0.0f;
    ::Wgpu::Buffer materialBuffer_;
    ::Wgpu::BindingLayout bindings_;
    ::Wgpu::Shader shader_;
    std::unique_ptr<Stage> constrain_, forces_;
};

/// Distances (> 0 in the region) of rows [0, rows) of a GpuSoA into a f32 buffer:
/// the GPU side of Geometry::evaluate, for checks and tools.
class GpuDistances final {
public:
    /// `source`: the interpreting machine or code generated for the program's structure
    /// (rebuilt when the structure changes).
    explicit GpuDistances(const GpuGeometry& geometry, DistanceSource source = interpretedDistance);
    ~GpuDistances();

    void evaluate(::Wgpu::CommandList& commands, ::Wgpu::GpuSoA& points, const ParticleLayout& layout, size_t rows,
                  ::Wgpu::Buffer& distances);

private:
    struct State;
    void build();

    const GpuGeometry& geometry_;
    DistanceSource source_;
    std::vector<Op> structure_;
    std::unique_ptr<State> state_;
};

}
