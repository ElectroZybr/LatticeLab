#pragma once

#include <Lattice/Tools/Logger.hpp>

#include <ParticleDynamics/include/ParticleAPI.hpp>
#include <ParticleDynamics/include/ParticleStorage.hpp>

#include <Lattice/Kernel/Node.hpp>

namespace Integrators {

class Verlet final : public ParticleDynamics::IntegratorAPI {
public:
    struct PrevForceX {using type = float;};
    struct PrevForceY {using type = float;};
    struct PrevForceZ {using type = float;};

    Verlet(Lattice::Node& branch) {}

    void configure(Lattice::Node& branch) {
        // интегратор требует для работы буфер. Если нет - исключение
        particles = branch.require<ParticleDynamics::ParticleStorage>();
        branch.bind("dt", &dt, 0, 0.1, true);
        particles->addCol<PrevForceX>();
        particles->addCol<PrevForceY>();
        particles->addCol<PrevForceZ>();
        configured = true;
    }

    void step() override;

    ~Verlet () {
        // if (settings)
        //     settings->unbind("verlet", "dt");
        if (configured && particles) {
            particles->removeCol<PrevForceX>();
            particles->removeCol<PrevForceY>();
            particles->removeCol<PrevForceZ>();
        }
        Logger::info("Verlet", "destroying object");
    }

private:
    void predict();
    void correct();

    float dt = 0.01;
    bool configured = false;

    Ref<ParticleDynamics::ParticleStorage> particles;
};
}