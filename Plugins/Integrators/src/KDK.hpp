#pragma once


#include <ParticleDynamics/include/ParticleAPI.hpp>

namespace Integrators {

class KDK final : public ParticleDynamics::IntegratorAPI {
public:
    KDK() = default;

    void step() override { }

    // void pipeline(StepContext& stepContext) const;
    // static void halfKick(AtomStorage& atomStorage, float dt);
    // static void drift(AtomStorage& atomStorage, float dt);
};

}
