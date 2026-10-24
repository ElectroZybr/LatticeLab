#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <Wgpu/Device.hpp>
#include <Wgpu/Resources.hpp>

#include "MeshData.hpp"

/// A triangle mesh on the device: vertex and index storage buffers (MeshVertex, u32),
/// shared by the objects that draw it. upload() replaces it; the generation grows when
/// the buffers are recreated, so bind groups know to follow.
class GpuMesh {
public:
    explicit GpuMesh(Wgpu::Device& device, std::string label = "mesh");
    GpuMesh(Wgpu::Device& device, const MeshData& data, std::string label = "mesh");
    virtual ~GpuMesh();
    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;

    void upload(const MeshData& data);

    /// Null while the mesh is empty.
    Wgpu::Buffer* vertices() const noexcept { return vertices_.get(); }
    Wgpu::Buffer* indices() const noexcept { return indices_.get(); }
    uint32_t indexCount() const noexcept { return indexCount_; }
    /// MeshEdge records; null while the mesh has none.
    Wgpu::Buffer* edges() const noexcept { return edges_.get(); }
    uint32_t edgeCount() const noexcept { return edgeCount_; }
    uint64_t generation() const noexcept { return generation_; }
    Wgpu::Device& device() const noexcept { return device_; }

protected:
    /// For meshes written on the device: buffers of at least these sizes.
    void reserve(uint64_t vertexBytes, uint64_t indexBytes);
    void reserveEdges(uint64_t bytes);
    void setIndexCount(uint32_t count) noexcept { indexCount_ = count; }
    void setEdgeCount(uint32_t count) noexcept { edgeCount_ = count; }

private:
    Wgpu::Device& device_;
    std::string label_;
    std::unique_ptr<Wgpu::Buffer> vertices_, indices_, edges_;
    uint32_t indexCount_ = 0, edgeCount_ = 0;
    uint64_t generation_ = 0;
};
