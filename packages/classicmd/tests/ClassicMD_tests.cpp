// Ordinary MD classes without NodeSystem, builders or package loading: the step pipeline
// is put together by hand as composition would.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <boundaries/Shape.hpp>
#include <classicmd/AtomData.hpp>
#include <classicmd/AtomStorage.hpp>
#include <classicmd/Boundary.hpp>
#include <classicmd/ClassicMD.hpp>
#include <integrators/Verlet.hpp>
#include <tasks/Queue.hpp>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

using Lattice::CallStage;
using Verlet = Integrators::Verlet;

/// A sphere of radius 1.5 in a box of 4: predict → constraints → forces → correct.
struct World {
    explicit World(std::chrono::milliseconds period = std::chrono::milliseconds(16))
        : simulation(atoms, step, commands, boundary, period) {
        constraints.setStages({&constrain});
        forces.setStages({&addForces});
        step.setStages({&predict, &constraints, &forces, &correct});
    }

    float kineticEnergy() const {
        float energy = 0.0f;
        for (size_t i = 0; i < atoms.mobileCount(); ++i) {
            const glm::vec3 v = atoms.get<Vel>(i);
            energy += 0.5f * glm::dot(v, v);
        }
        return energy;
    }

    ClassicMD::AtomStorage atoms;
    Boundaries::Space space{glm::vec3(-2.0f), glm::vec3(4.0f)};
    Boundaries::Cpu::Geometry geometry{Boundaries::Shape::sphere(1.5f)};
    Boundaries::Cpu::Walls walls{geometry, ClassicMD::particleLayout()};
    ClassicMD::Boundary boundary{atoms, space, walls};
    Verlet verlet{atoms};
    CallStage<Verlet> predict{verlet, &Verlet::predict};
    CallStage<ClassicMD::Boundary> constrain{boundary, &ClassicMD::Boundary::constrain};
    CallStage<ClassicMD::Boundary> addForces{boundary, &ClassicMD::Boundary::addForces};
    CallStage<Verlet> correct{verlet, &Verlet::correct};
    Lattice::Pipeline constraints, forces, step;
    Tasks::Queue commands;
    ClassicMD::ClassicMD simulation;
};

void simulationStepsThroughThePipeline() {
    World world;
    world.atoms.add({0, 0, 0}, {1, 10, 0});
    world.verlet.dt = 0.01f;
    check(world.atoms.has("Energy") && world.atoms.has("PrevForceX"), "atoms and columns");
    world.simulation.step();
    check(world.atoms.get<Pos>(0).y > 0.0f, "the step moved the atom");
    check(world.simulation.boundaries().space == &world.space &&
          world.simulation.boundaries().program == &world.geometry.program(), "the simulation shows its boundaries");
}

void atomsStayInsideAndKeepTheirEnergy() {
    World world;
    ClassicMD::fillRegion(world.atoms, world.space, world.geometry, 500, 0.05f, 2.0f);
    check(world.atoms.mobileCount() == 500, "the region is filled");
    world.verlet.dt = 0.005f;
    const float before = world.kineticEnergy();
    for (int i = 0; i < 4000; ++i)
        world.simulation.step();
    float deepest = 0.0f;
    for (size_t i = 0; i < world.atoms.size(); ++i)
        deepest = std::min(deepest, world.geometry.distance(world.atoms.get<Pos>(i)));
    check(deepest > -1e-3f, "no atom leaves the region");
    check(std::abs(world.kineticEnergy() - before) < 1e-3f * before, "elastic walls keep the energy");
}

void commandsRunOnTheSimulationThread() {
    World world{std::chrono::hours(1)};
    auto& simulation = world.simulation;
    simulation.commands().post([&] { world.verlet.dt = 0.5f; });
    check(world.verlet.dt == 0.5f, "without a running loop a command runs at once");

    std::thread worker([&] { simulation.run(); });
    while (!simulation.commands().serving())
        std::this_thread::yield();
    const auto thread = simulation.commands().call([] { return std::this_thread::get_id(); });
    check(thread == worker.get_id(), "commands run on the simulation thread");
    const auto before = simulation.commands().call([&] { return simulation.steps(); });
    simulation.commands().call([&] { simulation.step(); return 0; });
    check(simulation.commands().call([&] { return simulation.steps(); }) == before + 1,
          "an extra step runs between the loop's steps");
    simulation.requestStop();
    worker.join();
}

void loopStopsPromptly() {
    World world{std::chrono::hours(1)};
    auto& simulation = world.simulation;
    std::thread worker([&] { simulation.run(); });
    while (simulation.steps() == 0)
        std::this_thread::yield();
    const auto started = std::chrono::steady_clock::now();
    simulation.requestStop();
    worker.join();
    check(std::chrono::steady_clock::now() - started < std::chrono::seconds(1), "stop wakes the loop");
    check(simulation.steps() == 1, "one step per period");
}

void atomDataHasNamedRows() {
    ClassicMD::AtomData data;
    const size_t h = data.addRow("H");
    data.at<ClassicMD::Mass>(h) = 1.008f;
    check(data.requireRow("H") == h && data.columns() == 3, "atom data");
}
}

int main() {
    simulationStepsThroughThePipeline();
    atomsStayInsideAndKeepTheirEnergy();
    commandsRunOnTheSimulationThread();
    loopStopsPromptly();
    atomDataHasNamedRows();
    std::cout << "ClassicMD: pipeline, walls, commands, loop and atom data passed\n";
}
