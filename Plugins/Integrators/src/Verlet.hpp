#pragma once


#include <ParticleDynamics/include/ParticleAPI.hpp>
#include <ParticleDynamics/include/ParticleStorage.hpp>


namespace Integrators {

class Verlet final : public ParticleDynamics::IntegratorAPI {
public:
    struct PrevForceX {using type = float;};
    struct PrevForceY {using type = float;};
    struct PrevForceZ {using type = float;};

    explicit Verlet(ParticleDynamics::ParticleStorage& storage) : particles(&storage) {
        particles->addCol<PrevForceX>();
        particles->addCol<PrevForceY>();
        particles->addCol<PrevForceZ>();
    }

    Verlet(const Verlet&) = delete;
    Verlet& operator=(const Verlet&) = delete;

    float dt = 0.01f;

    void step() override;

    ~Verlet() {
        particles->removeCol<PrevForceX>();
        particles->removeCol<PrevForceY>();
        particles->removeCol<PrevForceZ>();
    }

private:
    void predict();
    void correct();

    ParticleDynamics::ParticleStorage* particles;
};
}
