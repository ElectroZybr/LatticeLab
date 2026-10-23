#pragma once

#include <ParticleDynamics/ParticleAPI.hpp>
#include <ParticleDynamics/ParticleStorage.hpp>
#include <glm/vec3.hpp>

namespace ClassicMD {

class ClassicMD final {
public:
    ClassicMD(ParticleDynamics::ParticleStorage& atoms,
              ParticleDynamics::IntegratorAPI& integrator)
        : integrator_(integrator) {
        atoms.add({0, 0, 0}, {1, 10, 0});
    }

    void step() { integrator_.step(); }

    glm::vec3 cellSize{};

private:
    ParticleDynamics::IntegratorAPI& integrator_;
};

}
