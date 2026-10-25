#include "MeshData.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>
#include <utility>

#include <glm/geometric.hpp>

std::vector<MeshEdge> meshEdges(const MeshData& mesh) {
    std::map<std::tuple<long, long, long>, uint32_t> welded;
    std::vector<uint32_t> weld(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const glm::vec3 p = mesh.vertices[i].position * 1e5f;
        weld[i] = welded.try_emplace({std::lround(p.x), std::lround(p.y), std::lround(p.z)}, uint32_t(welded.size()))
            .first->second;
    }

    std::map<std::pair<uint32_t, uint32_t>, size_t> index;
    std::vector<MeshEdge> edges;
    for (size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
        const glm::vec3 a = mesh.vertices[mesh.indices[t]].position;
        const glm::vec3 b = mesh.vertices[mesh.indices[t + 1]].position;
        const glm::vec3 c = mesh.vertices[mesh.indices[t + 2]].position;
        const glm::vec3 cross = glm::cross(b - a, c - a);
        const float length = glm::length(cross);
        const glm::vec4 normal(length > 0.0f ? cross / length : glm::vec3(0.0f), 1.0f);
        for (size_t k = 0; k < 3; ++k) {
            const uint32_t u = mesh.indices[t + k], v = mesh.indices[t + (k + 1) % 3];
            const auto key = std::minmax(weld[u], weld[v]);
            const auto [found, added] = index.try_emplace(key, edges.size());
            if (added)
                edges.push_back({.a = u, .b = v, .first = normal, .second = glm::vec4(0.0f)});
            else if (edges[found->second].second.w == 0.0f)
                edges[found->second].second = normal;
        }
    }
    return edges;
}
