#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <Boundaries/Laws.hpp>
#include <Boundaries/ParticleLayout.hpp>
#include <Wgpu/CommandList.hpp>
#include <Wgpu/GpuSoA.hpp>
#include <Wgpu/Resources.hpp>

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
    void addForces(::Wgpu::CommandList& commands, ::Wgpu::GpuSoA& particles, size_t rows);
    /// Constrain stage: move rows [0, rows) out of walls and reflect their velocities (Hard).
    void constrain(::Wgpu::CommandList& commands, ::Wgpu::GpuSoA& particles, size_t rows);

    static constexpr uint32_t WorkgroupSize = 64;

private:
    struct Stage;
    void record(::Wgpu::CommandList& commands, ::Wgpu::GpuSoA& particles, size_t rows, Stage& stage);

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
    explicit GpuDistances(const GpuGeometry& geometry);
    ~GpuDistances();

    void evaluate(::Wgpu::CommandList& commands, ::Wgpu::GpuSoA& points, const ParticleLayout& layout, size_t rows,
                  ::Wgpu::Buffer& distances);

private:
    struct State;
    const GpuGeometry& geometry_;
    std::unique_ptr<State> state_;
};

}
