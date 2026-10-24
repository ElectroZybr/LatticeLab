#include "Walls.hpp"

#include <algorithm>
#include <array>
#include <tuple>

#include <Lattice/Tools/Exception.hpp>

namespace Boundaries {

using Error = Lattice::Exception<Walls>;

namespace {

constexpr size_t Chunk = 256;

void requireColumns(const ParticleColumns& p, bool velocities, bool forces) {
    const size_t n = p.x.size();
    bool same = p.y.size() == n && p.z.size() == n;
    if (velocities) same &= p.vx.size() == n && p.vy.size() == n && p.vz.size() == n;
    if (forces) same &= p.fx.size() == n && p.fy.size() == n && p.fz.size() == n;
    if (!same)
        throw Error("particle columns differ in length");
}

}

Walls::Walls(const Geometry& geometry, std::vector<Material> materials)
    : geometry_(geometry), materials_(std::move(materials)) {
    if (materials_.empty())
        throw Error("walls need at least the default material");
    for (const auto& instruction : geometry_.program().code)
        if (isPrimitive(instruction.op) && instruction.material >= materials_.size())
            throw Error("geometry uses material {}, only {} defined", instruction.material, materials_.size());
    for (size_t i = 0; i < materials_.size(); ++i) {
        const auto& material = materials_[i];
        switch (material.law) {
            case Laws::Open::id:
                break;
            case Laws::Hard::id:
                if (material.params[0] < 0.0f || material.params[0] > 1.0f)
                    throw Error("material {}: restitution {} is outside [0, 1]", i, material.params[0]);
                break;
            case Laws::Soft::id:
                if (material.params[0] <= 0.0f || material.params[1] <= 0.0f || material.params[2] < 1.0f)
                    throw Error("material {}: soft wall needs stiffness > 0, range > 0, exponent >= 1", i);
                forceRange_ = std::max(forceRange_, Laws::Soft::range(material));
                break;
            default:
                throw Error("material {}: unknown law {}", i, material.law);
        }
    }
}

// Calls apply(index, contact, material) for every particle closer than `within` to a
// surface (or inside a wall), with the contact's distance and normal filled in.
template<class Apply>
void Walls::forNear(const ParticleColumns& p, float within, Apply&& apply) const {
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

void Walls::addForces(const ParticleColumns& p) const {
    if (forceRange_ <= 0.0f)
        return;
    requireColumns(p, false, true);
    forNear(p, forceRange_, [&](size_t i, const Contact& contact, const Material& material) {
        if (material.law != Laws::Soft::id)
            return;
        const glm::vec3 force = Laws::Soft::force(contact, material);
        p.fx[i] += force.x, p.fy[i] += force.y, p.fz[i] += force.z;
    });
}

void Walls::constrain(const ParticleColumns& p) const {
    requireColumns(p, true, false);
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
        for (int pass = 0; pass < ProjectionPasses; ++pass) {
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
