#pragma once

#include <Tasks/Queue.hpp>

#include "ParticleStorage.hpp"

namespace ParticleDynamics {

/// A simulation as seen by its readers (rendering, recording): its particles and the
/// command queue of the thread that owns them. The particles may be read only inside
/// a command of that queue: there the state is consistent and not being stepped.
struct ParticleSystemAPI {
    virtual ~ParticleSystemAPI() = default;
    virtual const ParticleStorage& particles() const = 0;
    virtual Tasks::Queue& commands() = 0;
};

}
