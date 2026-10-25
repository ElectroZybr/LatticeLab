#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

/// A vertex of a mesh: a GPU record (WGSL `Vertex` in Mesh.wgsl reads two vec4).
struct MeshVertex {
    glm::vec3 position;
    float pad0 = 0.0f;
    glm::vec3 normal;
    float pad1 = 0.0f;
};
static_assert(sizeof(MeshVertex) == 32, "MeshVertex is a GPU record: keep it 32 bytes");

/// An edge of a mesh with the normals of the faces on both sides (GPU record, `Edge` in
/// Mesh.wgsl). A boundary edge has one face: `second.w` is 0.
struct MeshEdge {
    uint32_t a, b;
    uint32_t pad0 = 0, pad1 = 0;
    glm::vec4 first;
    glm::vec4 second;
};
static_assert(sizeof(MeshEdge) == 48, "MeshEdge is a GPU record: keep it 48 bytes");

/// A triangle mesh on the host: three indices per triangle, counter-clockwise seen from
/// the side the normals point to.
struct MeshData {
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;

    size_t triangles() const noexcept { return indices.size() / 3; }
};

/// Every edge once, its sides found by position: faces with vertices of their own (flat
/// shading) still meet. An edge of more than two faces keeps the first two.
std::vector<MeshEdge> meshEdges(const MeshData& mesh);
