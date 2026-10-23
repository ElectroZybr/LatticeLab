// Plain integrators over plain storage: no runtime in this executable.
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <Integrators/KDK.hpp>
#include <Integrators/Verlet.hpp>

using namespace ParticleDynamics;

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void verletMovesMobileParticles() {
    ParticleStorage storage;
    storage.add({0, 0, 0}, {1, 2, 0});
    storage.add({5, 5, 5}, {1, 1, 1}, true);
    storage.at<InvMass>(0) = 1.0f;
    {
        Integrators::Verlet verlet{storage};
        verlet.dt = 0.5f;
        check(storage.has("PrevForceX"), "Verlet adds its columns");
        IntegratorAPI& integrator = verlet;
        integrator.step();
        const auto position = storage.get<Pos>(0);
        check(std::abs(position.x - 0.5f) < 1e-6f && std::abs(position.y - 1.0f) < 1e-6f, "mobile particle moved");
        check(storage.get<Pos>(1).x == 5.0f, "fixed particle stays");
    }
    check(!storage.has("PrevForceX"), "Verlet removes its columns");
}

void kdkIsAnIntegrator() {
    Integrators::KDK kdk;
    IntegratorAPI& integrator = kdk;
    integrator.step();
}
}

int main() {
    verletMovesMobileParticles();
    kdkIsAnIntegrator();
    std::cout << "Integrators: Verlet and KDK passed\n";
}
