#pragma once

#include <cstdint>

#include <boundaries/Program.hpp>
#include <boundaries/Space.hpp>
#include <tasks/Queue.hpp>

#include "ParticleStorage.hpp"

namespace ParticleDynamics {

/// The box and the walls of a simulation; `version` grows when either changes.
struct BoundaryView {
    const Boundaries::Space* space = nullptr;
    const Boundaries::Program* program = nullptr;
    uint64_t version = 0;
};

/// A simulation as seen by its readers (rendering, recording): its particles, its
/// boundaries and the command queue of the thread that owns them. Both may be read only
/// inside a command of that queue: there the state is consistent and not being stepped.
struct ParticleSystemAPI {
    virtual ~ParticleSystemAPI() = default;
    virtual const ParticleStorage& particles() const = 0;
    /// No space when the simulation has no boundaries.
    virtual BoundaryView boundaries() const { return {}; }
    virtual Tasks::Queue& commands() = 0;
};

}
