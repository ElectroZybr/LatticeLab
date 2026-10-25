#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include <boundaries/Program.hpp>
#include <wgpu/Resources.hpp>

#include "GpuBoundaries.hpp"
#include "GpuMesh.hpp"
#include "SurfaceNets.hpp"

/// The surface of a geometry program as a mesh, built on the GPU by surface nets over the
/// box of its space: bricks of 8³ cells, only those the surface may cross are sampled.
/// update() rebuilds it when the boundaries or the resolution changed; without boundaries
/// the mesh is empty.
class SdfMesh final : public GpuMesh {
public:
    static constexpr uint32_t Brick = 8;

    SdfMesh(Wgpu::Device& device, const GpuBoundaries& boundaries, uint32_t resolution = 128);
    ~SdfMesh() override;

    bool update();
    void build();

    /// Cells along the longest side of the space.
    uint32_t resolution;

    SurfaceGrid grid() const noexcept { return grid_; }
    uint32_t activeBricks() const noexcept { return activeBricks_; }
    uint32_t vertexCount() const noexcept { return vertexCount_; }

private:
    struct Pipelines;
    void prepare(const Boundaries::Wgpu::GpuGeometry& geometry, const Boundaries::Space& space);
    bool run(const Boundaries::Wgpu::GpuGeometry& geometry);

    const GpuBoundaries& boundaries_;
    Wgpu::Device& device_;
    Wgpu::BindingLayout layout_;
    Wgpu::Buffer params_, counters_;
    std::vector<Boundaries::Op> structure_;
    std::unique_ptr<Pipelines> pipelines_;
    std::unique_ptr<Wgpu::Buffer> slotOf_, brickOf_, slotData_;
    std::unique_ptr<Wgpu::BindingSet> set_;
    SurfaceGrid grid_{};
    glm::uvec3 bricks_{0u};
    std::array<uint32_t, 4> capacity_{};
    uint64_t builtVersion_ = ~0ull;
    uint32_t builtResolution_ = 0;
    uint32_t activeBricks_ = 0, vertexCount_ = 0;
};
