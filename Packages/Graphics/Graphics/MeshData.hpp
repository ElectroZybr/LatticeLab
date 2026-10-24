#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

/// A vertex of a mesh: a GPU record (WGSL `Vertex` in Mesh.wgsl reads two vec4).
struct MeshVertex {
    glm::vec3 position;
    float pad0 = 0.0f;
    glm::vec3 normal;
    float pad1 = 0.0f;
};
static_assert(sizeof(MeshVertex) == 32, "MeshVertex is a GPU record: keep it 32 bytes");

/// A triangle mesh on the host: three indices per triangle, counter-clockwise seen from
/// the side the normals point to.
struct MeshData {
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;

    size_t triangles() const noexcept { return indices.size() / 3; }
};
