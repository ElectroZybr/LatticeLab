// Plain particle storage and spatial index: no runtime in this executable.
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <particledynamics/ParticleAPI.hpp>
#include <particledynamics/ParticleStorage.hpp>
#include <particledynamics/SpatialGrid.hpp>

using namespace ParticleDynamics;
static_assert(std::is_default_constructible_v<ParticleStorage>);

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void storageKeepsMobileParticlesFirst() {
    ParticleStorage storage;
    check(storage.empty() && storage.columns() == 10, "default columns");
    const size_t fixed = storage.add({1, 0, 0}, {0, 0, 0}, true);
    const size_t mobile = storage.add({2, 0, 0}, {1, 0, 0});
    check(storage.size() == 2 && storage.mobileCount() == 1, "counts");
    check(mobile == 0 && storage.get<Pos>(0).x == 2.0f, "mobile particle is first");
    check(storage.isFixed(1) && storage.get<Pos>(1).x == 1.0f, "fixed particle is last");
    (void)fixed;

    storage.setFixed(0, true);
    check(storage.mobileCount() == 0 && storage.isFixed(0), "set fixed");
    storage.setFixed(0, false);
    check(storage.mobileCount() == 1, "set mobile");

    storage.remove(0);
    check(storage.size() == 1 && storage.mobileCount() == 0 && storage.get<Pos>(0).x == 1.0f, "remove");
}

void spatialGridGroupsParticlesByCell() {
    ParticleStorage storage;
    storage.add({0.5f, 0.5f, 0.5f}, {});
    storage.add({0.6f, 0.4f, 0.5f}, {});
    storage.add({2.5f, 2.5f, 2.5f}, {});

    SpatialGrid grid{storage};
    grid.setCellSize(1.0f);
    grid.setSize({4, 4, 4});
    check(grid.cellCount() == 6 * 6 * 6, "grid has ghost layers");
    grid.rebuild();

    auto first = grid.particlesInCell(1, 1, 1);
    check(first.size() == 2, "two particles share a cell");
    check(std::ranges::find(first, 0u) != first.end() && std::ranges::find(first, 1u) != first.end(), "cell members");
    check(grid.particlesInCell(3, 3, 3).size() == 1, "third particle");

    bool failed = false;
    try { grid.setCellSize(0); } catch (const std::invalid_argument&) { failed = true; }
    check(failed, "invalid cell size");
}
}

int main() {
    storageKeepsMobileParticlesFirst();
    spatialGridGroupsParticlesByCell();
    std::cout << "ParticleDynamics: storage and spatial grid passed\n";
}
