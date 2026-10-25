#pragma once

#include <span>
#include <vector>

#include <glm/vec3.hpp>

#include "MeshData.hpp"

/// A box split into cells; samples are taken at the (cells + 1)³ corners, x fastest.
struct SurfaceGrid {
    glm::vec3 origin;
    glm::vec3 cellSize;
    glm::uvec3 cells;

    glm::uvec3 corners() const noexcept { return cells + 1u; }
    size_t sampleCount() const noexcept { const auto c = corners(); return size_t(c.x) * c.y * c.z; }
    glm::vec3 corner(glm::uvec3 index) const noexcept { return origin + glm::vec3(index) * cellSize; }
};

/// Distances at the grid corners; > 0 in the region, < 0 in a wall.
template<class Distance>
std::vector<float> sampleGrid(const SurfaceGrid& grid, Distance&& distance) {
    std::vector<float> samples;
    samples.reserve(grid.sampleCount());
    const glm::uvec3 n = grid.corners();
    for (uint32_t z = 0; z < n.z; ++z)
        for (uint32_t y = 0; y < n.y; ++y)
            for (uint32_t x = 0; x < n.x; ++x)
                samples.push_back(distance(grid.corner({x, y, z})));
    return samples;
}

/// The surface d = 0 by surface nets: a vertex per cell the surface crosses, at the mean of
/// the edge crossings, and a quad per crossed grid edge between the four cells around it.
/// Faces and normals point into the region (d grows). Edges on the grid border make no
/// faces, so the mesh is open where the surface leaves the grid.
MeshData surfaceNets(const SurfaceGrid& grid, std::span<const float> samples);
