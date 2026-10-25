#pragma once

#include <cstdint>
#include <random>
#include <string>

#include <boundaries/ParticleLayout.hpp>
#include <boundaries/Space.hpp>
#include <boundariescpu/Geometry.hpp>
#include <boundariescpu/Walls.hpp>
#include <boundariescpu/Wrap.hpp>
#include <Lattice/tools/TypeName.hpp>
#include <particledynamics/ParticleStorage.hpp>
#include <particledynamics/ParticleSystemAPI.hpp>

namespace ClassicMD {

/// Columns of the particle storage the boundaries read and write.
inline Boundaries::ParticleLayout particleLayout() {
    const auto name = [](auto tag) { return std::string(Lattice::typeName<decltype(tag)>()); };
    return {name(Pos::X{}), name(Pos::Y{}), name(Pos::Z{}), name(Vel::X{}), name(Vel::Y{}), name(Vel::Z{}),
            name(Force::X{}), name(Force::Y{}), name(Force::Z{})};
}

/// The space and the walls acting on the atoms: constrain() after the positions move
/// (wrap along periodic axes, out of hard walls), addForces() with the other forces.
class Boundary final {
public:
    Boundary(ParticleDynamics::ParticleStorage& atoms, const Boundaries::Space& space,
             const Boundaries::Cpu::Walls& walls)
        : atoms_(atoms), space_(space), walls_(walls) {}

    void constrain() {
        const size_t mobile = atoms_.mobileCount();
        Boundaries::Cpu::wrapPositions(space_, atoms_, walls_.layout(), mobile);
        walls_.constrain(atoms_, mobile);
    }

    void addForces() { walls_.addForces(atoms_, atoms_.mobileCount()); }

    ParticleDynamics::BoundaryView view() const {
        return {.space = &space_, .program = &walls_.geometry().program(), .version = version_};
    }

private:
    ParticleDynamics::ParticleStorage& atoms_;
    const Boundaries::Space& space_;
    const Boundaries::Cpu::Walls& walls_;
    uint64_t version_ = 1;
};

/// `count` atoms at random places of the region at least `margin` from its walls, with
/// random velocities up to `speed` and unit mass.
inline void fillRegion(ParticleDynamics::ParticleStorage& atoms, const Boundaries::Space& space,
                       const Boundaries::Cpu::Geometry& geometry, size_t count, float margin, float speed,
                       uint32_t seed = 1) {
    std::mt19937 random(seed);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f), velocity(-speed, speed);
    for (size_t placed = 0, tries = 0; placed < count && tries < count * 100; ++tries) {
        const glm::vec3 p = space.origin() + glm::vec3(unit(random), unit(random), unit(random)) * space.size();
        if (geometry.distance(p) < margin)
            continue;
        const size_t i = atoms.add(p, {velocity(random), velocity(random), velocity(random)});
        atoms.at<InvMass>(i) = 1.0f;
        ++placed;
    }
}

}
