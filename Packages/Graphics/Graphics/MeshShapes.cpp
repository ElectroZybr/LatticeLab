#include "MeshShapes.hpp"

#include <cmath>
#include <numbers>

#include <glm/geometric.hpp>
#include <Lattice/Tools/Exception.hpp>

namespace MeshShapes {

namespace {

using Error = Lattice::Exception<MeshData>;

constexpr float Tau = 2.0f * std::numbers::pi_v<float>;

void quad(MeshData& mesh, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    mesh.indices.insert(mesh.indices.end(), {a, b, c, a, c, d});
}

}

MeshData box(glm::vec3 h) {
    MeshData mesh;
    // Per face: the normal and two axes spanning it, right-handed (u × v = normal).
    const glm::vec3 faces[6][3] = {
        {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}}, {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
        {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}}, {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
        {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}}, {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
    };
    for (const auto& [n, u, v] : faces) {
        const auto base = uint32_t(mesh.vertices.size());
        for (const auto [su, sv] : {std::pair{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}})
            mesh.vertices.push_back({.position = (n + su * u + sv * v) * h, .normal = n});
        quad(mesh, base, base + 1, base + 2, base + 3);
    }
    return mesh;
}

MeshData sphere(float radius, uint32_t segments, uint32_t rings) {
    if (segments < 3 || rings < 2)
        throw Error("sphere needs at least 3 segments and 2 rings, got {} and {}", segments, rings);
    MeshData mesh;
    mesh.vertices.push_back({.position = {0, radius, 0}, .normal = {0, 1, 0}});
    for (uint32_t r = 1; r < rings; ++r) {
        const float polar = std::numbers::pi_v<float> * float(r) / float(rings);
        for (uint32_t s = 0; s < segments; ++s) {
            const float azimuth = Tau * float(s) / float(segments);
            const glm::vec3 n{std::sin(polar) * std::cos(azimuth), std::cos(polar), -std::sin(polar) * std::sin(azimuth)};
            mesh.vertices.push_back({.position = n * radius, .normal = n});
        }
    }
    const auto south = uint32_t(mesh.vertices.size());
    mesh.vertices.push_back({.position = {0, -radius, 0}, .normal = {0, -1, 0}});

    const auto at = [&](uint32_t ring, uint32_t s) { return 1 + (ring - 1) * segments + s % segments; };
    for (uint32_t s = 0; s < segments; ++s) {
        mesh.indices.insert(mesh.indices.end(), {0, at(1, s), at(1, s + 1)});
        for (uint32_t r = 1; r + 1 < rings; ++r)
            quad(mesh, at(r, s), at(r + 1, s), at(r + 1, s + 1), at(r, s + 1));
        mesh.indices.insert(mesh.indices.end(), {south, at(rings - 1, s + 1), at(rings - 1, s)});
    }
    return mesh;
}

MeshData torus(float major, float minor, uint32_t segments, uint32_t sides) {
    if (segments < 3 || sides < 3)
        throw Error("torus needs at least 3 segments and 3 sides, got {} and {}", segments, sides);
    MeshData mesh;
    mesh.vertices.reserve(size_t(segments) * sides);
    for (uint32_t s = 0; s < segments; ++s) {
        const float around = Tau * float(s) / float(segments);
        const glm::vec3 radial{std::cos(around), 0, -std::sin(around)};
        for (uint32_t t = 0; t < sides; ++t) {
            const float tube = Tau * float(t) / float(sides);
            const glm::vec3 n = std::cos(tube) * radial + std::sin(tube) * glm::vec3(0, 1, 0);
            mesh.vertices.push_back({.position = major * radial + minor * n, .normal = n});
        }
    }
    const auto at = [&](uint32_t s, uint32_t t) { return (s % segments) * sides + t % sides; };
    mesh.indices.reserve(size_t(segments) * sides * 6);
    for (uint32_t s = 0; s < segments; ++s)
        for (uint32_t t = 0; t < sides; ++t)
            quad(mesh, at(s, t), at(s + 1, t), at(s + 1, t + 1), at(s, t + 1));
    return mesh;
}

}
