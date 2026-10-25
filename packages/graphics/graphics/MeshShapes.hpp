#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

#include "MeshData.hpp"

/// Meshes of simple shapes around the origin, normals outward: demos, tests, benchmarks.
namespace MeshShapes {

/// Flat faces: four vertices of their own per face.
MeshData box(glm::vec3 halfExtents);
/// `segments` around the y axis, `rings` from pole to pole (one vertex per pole).
MeshData sphere(float radius, uint32_t segments, uint32_t rings);
/// Around the y axis: `segments` along the ring, `sides` around the tube; 2 · segments · sides triangles.
MeshData torus(float majorRadius, float minorRadius, uint32_t segments, uint32_t sides);

}
