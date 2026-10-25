// Plain integrators over plain storage: no runtime in this executable.
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <integrators/Verlet.hpp>

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
        verlet.predict();
        verlet.correct();
        const auto position = storage.get<Pos>(0);
        check(std::abs(position.x - 0.5f) < 1e-6f && std::abs(position.y - 1.0f) < 1e-6f, "mobile particle moved");
        check(storage.get<Pos>(1).x == 5.0f, "fixed particle stays");
    }
    check(!storage.has("PrevForceX"), "Verlet removes its columns");
}

/// Under a constant force the position follows x = v·t + ½(F/m)·t² exactly.
void verletFollowsConstantForce() {
    ParticleStorage storage;
    storage.add({0, 0, 0}, {1, 0, 0});
    storage.at<InvMass>(0) = 0.5f;
    Integrators::Verlet verlet{storage};
    verlet.dt = 0.1f;
    const auto force = [&] { storage.at<Force::Y>(0) += 4.0f; };
    force();
    for (int step = 0; step < 50; ++step) {
        verlet.predict();
        force();
        verlet.correct();
    }
    const float t = 5.0f;
    const auto position = storage.get<Pos>(0);
    const auto velocity = storage.get<Vel>(0);
    check(std::abs(position.x - t) < 1e-4f && std::abs(position.y - 0.5f * 2.0f * t * t) < 1e-3f, "position under a constant force");
    check(std::abs(velocity.y - 2.0f * t) < 1e-4f, "velocity under a constant force");
}
}

int main() {
    verletMovesMobileParticles();
    verletFollowsConstantForce();
    std::cout << "Integrators: Verlet passed\n";
}
