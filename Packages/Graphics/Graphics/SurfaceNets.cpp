#include "SurfaceNets.hpp"

#include <array>
#include <cstdint>

#include <glm/geometric.hpp>
#include <Lattice/Tools/Exception.hpp>

namespace {

constexpr uint32_t NoVertex = ~0u;

constexpr std::array<glm::uvec3, 8> Corners{{
    {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}}};

constexpr std::array<std::array<uint32_t, 2>, 12> Edges{{
    {0, 1}, {2, 3}, {4, 5}, {6, 7},
    {0, 2}, {1, 3}, {4, 6}, {5, 7},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}}};

}

MeshData surfaceNets(const SurfaceGrid& grid, std::span<const float> samples) {
    using Error = Lattice::Exception<SurfaceGrid>;
    if (samples.size() != grid.sampleCount())
        throw Error("{} samples for a grid of {} corners", samples.size(), grid.sampleCount());

    const glm::uvec3 n = grid.corners();
    const auto sample = [&](glm::uvec3 p) { return samples[(size_t(p.z) * n.y + p.y) * n.x + p.x]; };
    const auto cellIndex = [&](glm::uvec3 c) { return (size_t(c.z) * grid.cells.y + c.y) * grid.cells.x + c.x; };

    MeshData mesh;
    std::vector<uint32_t> vertexOf(size_t(grid.cells.x) * grid.cells.y * grid.cells.z, NoVertex);
    for (uint32_t z = 0; z < grid.cells.z; ++z)
        for (uint32_t y = 0; y < grid.cells.y; ++y)
            for (uint32_t x = 0; x < grid.cells.x; ++x) {
                const glm::uvec3 cell{x, y, z};
                std::array<float, 8> d;
                bool inside = false, outside = false;
                for (size_t k = 0; k < 8; ++k) {
                    d[k] = sample(cell + Corners[k]);
                    (d[k] > 0.0f ? inside : outside) = true;
                }
                if (!inside || !outside)
                    continue;

                glm::vec3 sum{0.0f};
                uint32_t crossings = 0;
                for (const auto [a, b] : Edges) {
                    if ((d[a] > 0.0f) == (d[b] > 0.0f))
                        continue;
                    const float t = d[a] / (d[a] - d[b]);
                    sum += glm::mix(glm::vec3(Corners[a]), glm::vec3(Corners[b]), t);
                    ++crossings;
                }
                const glm::vec3 local = sum / float(crossings);
                // Gradient of the trilinear field at the vertex: points into the region.
                const glm::vec3 u = local, v = 1.0f - local;
                const glm::vec3 gradient{
                    v.y * v.z * (d[1] - d[0]) + u.y * v.z * (d[3] - d[2]) + v.y * u.z * (d[5] - d[4]) + u.y * u.z * (d[7] - d[6]),
                    v.x * v.z * (d[2] - d[0]) + u.x * v.z * (d[3] - d[1]) + v.x * u.z * (d[6] - d[4]) + u.x * u.z * (d[7] - d[5]),
                    v.x * v.y * (d[4] - d[0]) + u.x * v.y * (d[5] - d[1]) + v.x * u.y * (d[6] - d[2]) + u.x * u.y * (d[7] - d[3])};
                const glm::vec3 world = gradient / grid.cellSize;
                const float length = glm::length(world);
                vertexOf[cellIndex(cell)] = uint32_t(mesh.vertices.size());
                mesh.vertices.push_back({.position = grid.origin + (glm::vec3(cell) + local) * grid.cellSize,
                    .normal = length > 0.0f ? world / length : glm::vec3(0.0f, 1.0f, 0.0f)});
            }

    // An edge along axis a from p to p + e_a is shared by the cells p − e_b − e_c … p; its
    // quad faces +a when the region lies at the far end. (a, b, c) is a cyclic triple.
    for (uint32_t a = 0; a < 3; ++a) {
        const uint32_t b = (a + 1) % 3, c = (a + 2) % 3;
        glm::uvec3 e{0u};
        e[a] = 1;
        for (uint32_t z = 0; z < n.z; ++z)
            for (uint32_t y = 0; y < n.y; ++y)
                for (uint32_t x = 0; x < n.x; ++x) {
                    const glm::uvec3 p{x, y, z};
                    if (p[a] + 1 >= n[a] || p[b] == 0 || p[c] == 0 || p[b] + 1 >= n[b] || p[c] + 1 >= n[c])
                        continue;
                    const bool near = sample(p) > 0.0f, far = sample(p + e) > 0.0f;
                    if (near == far)
                        continue;
                    const auto vertex = [&](uint32_t db, uint32_t dc) {
                        glm::uvec3 cell = p;
                        cell[b] -= db;
                        cell[c] -= dc;
                        return vertexOf[cellIndex(cell)];
                    };
                    const std::array quad{vertex(1, 1), vertex(0, 1), vertex(0, 0), vertex(1, 0)};
                    if (far)
                        mesh.indices.insert(mesh.indices.end(), {quad[0], quad[1], quad[2], quad[0], quad[2], quad[3]});
                    else
                        mesh.indices.insert(mesh.indices.end(), {quad[0], quad[2], quad[1], quad[0], quad[3], quad[2]});
                }
    }
    return mesh;
}
