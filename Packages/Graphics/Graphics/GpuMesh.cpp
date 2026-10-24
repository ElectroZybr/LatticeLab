#include "GpuMesh.hpp"

#include <algorithm>
#include <span>

#include <Lattice/Tools/Exception.hpp>

namespace {

/// A storage buffer of at least `bytes`: kept while large enough, else recreated.
bool fit(Wgpu::Device& device, std::unique_ptr<Wgpu::Buffer>& buffer, uint64_t bytes, const std::string& label) {
    if (buffer && buffer->size() >= bytes)
        return false;
    buffer = std::make_unique<Wgpu::Buffer>(device, Wgpu::BufferDesc{.size = bytes,
        .usage = Wgpu::BufferUsage::Storage | Wgpu::BufferUsage::CopyDestination | Wgpu::BufferUsage::CopySource,
        .label = label});
    return true;
}

}

GpuMesh::GpuMesh(Wgpu::Device& device, std::string label) : device_(device), label_(std::move(label)) {}

GpuMesh::GpuMesh(Wgpu::Device& device, const MeshData& data, std::string label) : GpuMesh(device, std::move(label)) {
    upload(data);
}

GpuMesh::~GpuMesh() = default;

void GpuMesh::upload(const MeshData& data) {
    using Error = Lattice::Exception<GpuMesh>;
    if (data.indices.size() % 3)
        throw Error("mesh '{}': {} indices are not whole triangles", label_, data.indices.size());
    for (uint32_t index : data.indices)
        if (index >= data.vertices.size())
            throw Error("mesh '{}': index {} of {} vertices", label_, index, data.vertices.size());
    indexCount_ = uint32_t(data.indices.size());
    edgeCount_ = 0;
    if (data.indices.empty())
        return;
    reserve(data.vertices.size() * sizeof(MeshVertex), data.indices.size() * sizeof(uint32_t));
    device_.writeBuffer(*vertices_, 0, std::as_bytes(std::span(data.vertices)));
    device_.writeBuffer(*indices_, 0, std::as_bytes(std::span(data.indices)));
    const std::vector<MeshEdge> edges = meshEdges(data);
    edgeCount_ = uint32_t(edges.size());
    reserveEdges(edges.size() * sizeof(MeshEdge));
    device_.writeBuffer(*edges_, 0, std::as_bytes(std::span(edges)));
}

void GpuMesh::reserve(uint64_t vertexBytes, uint64_t indexBytes) {
    bool recreated = fit(device_, vertices_, std::max<uint64_t>(vertexBytes, 4), label_ + "/vertices");
    recreated |= fit(device_, indices_, std::max<uint64_t>(indexBytes, 4), label_ + "/indices");
    if (recreated)
        ++generation_;
}

void GpuMesh::reserveEdges(uint64_t bytes) {
    if (fit(device_, edges_, std::max<uint64_t>(bytes, sizeof(MeshEdge)), label_ + "/edges"))
        ++generation_;
}
