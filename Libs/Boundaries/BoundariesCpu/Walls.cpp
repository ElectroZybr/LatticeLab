#include "Walls.hpp"

#include <algorithm>
#include <array>
#include <tuple>

#include <Boundaries/Materials.hpp>
#include <Lattice/Tools/Exception.hpp>

namespace Boundaries::Cpu {

using Error = Lattice::Exception<Walls>;

namespace {

constexpr size_t Chunk = 256;

// Calls apply(index, contact, material) for every particle closer than `within` to a
// surface (or inside a wall), with the contact's distance and normal filled in.
template<class Apply>
void forNear(const Geometry& geometry_, const std::vector<Material>& materials_, std::span<float> px,
             std::span<float> py, std::span<float> pz, float within, Apply&& apply) {
    struct { std::span<float> x, y, z; } p{px, py, pz};
    std::array<float, Chunk> distance;
    std::array<uint32_t, Chunk> material;
    std::array<uint32_t, Chunk> selected;
    std::array<float, Chunk> sx, sy, sz, nx, ny, nz;

    for (size_t begin = 0; begin < p.x.size(); begin += Chunk) {
        const size_t n = std::min(Chunk, p.x.size() - begin);
        geometry_.evaluate(p.x.subspan(begin, n), p.y.subspan(begin, n), p.z.subspan(begin, n),
            std::span(distance).first(n), std::span(material).first(n));

        size_t count = 0;
        for (size_t i = 0; i < n; ++i)
            if (distance[i] < within)
                selected[count++] = uint32_t(i);
        if (!count)
            continue;

        for (size_t j = 0; j < count; ++j) {
            const size_t i = begin + selected[j];
            sx[j] = p.x[i], sy[j] = p.y[i], sz[j] = p.z[i];
        }
        geometry_.normals(std::span(sx).first(count), std::span(sy).first(count), std::span(sz).first(count),
            std::span(nx).first(count), std::span(ny).first(count), std::span(nz).first(count));

        for (size_t j = 0; j < count; ++j) {
            const size_t local = selected[j];
            Contact contact{
                .position = {sx[j], sy[j], sz[j]},
                .velocity = {},
                .distance = distance[local],
                .normal = {nx[j], ny[j], nz[j]},
            };
            apply(begin + local, contact, materials_[material[local]]);
        }
    }
}


}

Walls::Walls(const Geometry& geometry, ParticleLayout layout, std::vector<Material> materials)
    : geometry_(geometry), layout_(std::move(layout)), materials_(std::move(materials)),
      forceRange_(checkMaterials(geometry_.program(), materials_)) {}

Walls::Columns Walls::columns(StdData::SoA& particles, size_t begin, size_t end, bool velocities, bool forces) const {
    if (begin > end || end > particles.size())
        throw Error("rows [{}, {}) of a table with {}", begin, end, particles.size());
    const auto column = [&](const std::string& name) { return particles.span<float>(name).subspan(begin, end - begin); };
    Columns c{column(layout_.x), column(layout_.y), column(layout_.z)};
    if (velocities)
        c.vx = column(layout_.vx), c.vy = column(layout_.vy), c.vz = column(layout_.vz);
    if (forces)
        c.fx = column(layout_.fx), c.fy = column(layout_.fy), c.fz = column(layout_.fz);
    return c;
}

void Walls::addForces(StdData::SoA& particles, size_t begin, size_t end) const {
    if (forceRange_ <= 0.0f)
        return;
    const Columns p = columns(particles, begin, end, false, true);
    forNear(geometry_, materials_, p.x, p.y, p.z, forceRange_, [&](size_t i, const Contact& contact, const Material& material) {
        if (material.law != Laws::Soft::id)
            return;
        const glm::vec3 force = Laws::Soft::force(contact, material);
        p.fx[i] += force.x, p.fy[i] += force.y, p.fz[i] += force.z;
    });
}

void Walls::constrain(StdData::SoA& particles, size_t first, size_t last) const {
    const Columns p = columns(particles, first, last, true, false);
    std::array<float, Chunk> distance;
    std::array<uint32_t, Chunk> material;
    // Particles of the chunk in Hard walls: their index, position, distance, normal.
    std::array<uint32_t, Chunk> index;
    std::array<float, Chunk> sx, sy, sz, sd, nx, ny, nz;

    for (size_t begin = 0; begin < p.x.size(); begin += Chunk) {
        const size_t n = std::min(Chunk, p.x.size() - begin);
        geometry_.evaluate(p.x.subspan(begin, n), p.y.subspan(begin, n), p.z.subspan(begin, n),
            std::span(distance).first(n), std::span(material).first(n));

        size_t count = 0;
        for (size_t i = 0; i < n; ++i)
            if (distance[i] < 0.0f && materials_[material[i]].law == Laws::Hard::id) {
                const size_t k = begin + i;
                index[count] = uint32_t(i);
                sx[count] = p.x[k], sy[count] = p.y[k], sz[count] = p.z[k], sd[count] = distance[i];
                ++count;
            }
        if (!count)
            continue;

        // Back onto the surface along the normal, all at once; blends give a distance
        // bound, so repeat while some are still inside (rarely more than one pass).
        const auto positions = [&] { return std::tuple{std::span(sx).first(count), std::span(sy).first(count),
            std::span(sz).first(count)}; };
        for (int pass = 0; pass < Laws::Hard::ProjectionPasses; ++pass) {
            auto [x, y, z] = positions();
            geometry_.normals(x, y, z, std::span(nx).first(count), std::span(ny).first(count), std::span(nz).first(count));
            bool inside = false;
            for (size_t j = 0; j < count; ++j)
                if (sd[j] < 0.0f) {
                    sx[j] -= nx[j] * sd[j], sy[j] -= ny[j] * sd[j], sz[j] -= nz[j] * sd[j];
                    inside = true;
                }
            if (!inside)
                break;
            geometry_.evaluate(x, y, z, std::span(sd).first(count));
        }
        {
            auto [x, y, z] = positions();
            geometry_.normals(x, y, z, std::span(nx).first(count), std::span(ny).first(count), std::span(nz).first(count));
        }

        for (size_t j = 0; j < count; ++j) {
            const size_t k = begin + index[j];
            const glm::vec3 velocity = Laws::Hard::reflect({p.vx[k], p.vy[k], p.vz[k]}, {nx[j], ny[j], nz[j]},
                materials_[material[index[j]]]);
            p.x[k] = sx[j], p.y[k] = sy[j], p.z[k] = sz[j];
            p.vx[k] = velocity.x, p.vy[k] = velocity.y, p.vz[k] = velocity.z;
        }
    }
}

}
