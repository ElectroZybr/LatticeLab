#pragma once


#include <particledynamics/ParticleStorage.hpp>


namespace Integrators {

/// Velocity Verlet in two halves around the forces of a step: predict() moves the mobile
/// particles and keeps their forces as the previous ones, then clears them; the forces are
/// added anew; correct() updates the velocities with the mean of the old and new forces.
class Verlet final {
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

    void predict();
    void correct();

    ~Verlet() {
        particles->removeCol<PrevForceX>();
        particles->removeCol<PrevForceY>();
        particles->removeCol<PrevForceZ>();
    }

private:
    ParticleDynamics::ParticleStorage* particles;
};
}
