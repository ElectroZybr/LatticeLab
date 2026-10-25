#include <Lattice/Composition.hpp>
#include <boundaries/Shape.hpp>
#include <classicmd/AtomData.hpp>
#include <classicmd/AtomStorage.hpp>
#include <classicmd/Boundary.hpp>
#include <classicmd/ClassicMD.hpp>
#include <integrators/Verlet.hpp>
#include <tasks/Queue.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "ClassicMD",
    .version = {1, 0, 0},
};

namespace {

using Lattice::CallStage;
using Lattice::Pipeline;
using Lattice::PipelineAPI;

// A rounded cylindrical vessel with an obstacle in the middle, until scenes are loaded.
Boundaries::Shape vessel() {
    using Boundaries::Shape;
    const auto container = Shape::cylinder(1.4f, 1.4f).rounded(0.3f);
    const auto obstacle = Shape::smoothUnion(Shape::sphere(0.5f),
        Shape::capsule({-1.2f, 0.0f, 0.0f}, {1.2f, 0.0f, 0.0f}, 0.15f), 0.25f);
    return container - obstacle;
}

}

extern "C" void compose(NodeBuild root) {
    // Commands of the simulation thread: every export below runs through them, so
    // reads, writes and calls never overlap a step.
    auto commands = root.emplace<Tasks::Queue>("commands");

    auto data = root.emplace<ClassicMD::AtomData, StdData::NamedSoA, Lattice::Table>("data");
    data.executeOn(commands);
    Lattice::exportTable(data);
    auto atoms = root.emplace<ClassicMD::AtomStorage, ParticleDynamics::ParticleStorage, Lattice::Table>("atoms");
    atoms.executeOn(commands);
    Lattice::exportTable(atoms);

    auto space = root.emplace<Boundaries::Space>("space", glm::vec3(-2.0f), glm::vec3(4.0f));
    auto geometry = root.emplace<Boundaries::Cpu::Geometry>("geometry", vessel());
    auto walls = root.emplace<Boundaries::Cpu::Walls>("walls", geometry, ClassicMD::particleLayout());
    auto boundary = root.emplace<ClassicMD::Boundary>("boundary", atoms, space, walls);
    ClassicMD::fillRegion(*atoms, *space, *geometry, 1000, 0.05f, 1.0f);

    auto integrator = root.emplace<Integrators::Verlet>("integrator", atoms);
    integrator.executeOn(commands);
    integrator.param("dt", &Integrators::Verlet::dt);

    // The step: the integrator's halves around constraints and forces, each a node.
    auto step = root.branch("step");
    step.emplace<CallStage<Integrators::Verlet>, PipelineAPI>("predict", integrator, &Integrators::Verlet::predict);
    auto constraints = step.branch("constraints");
    constraints.emplace<CallStage<ClassicMD::Boundary>, PipelineAPI>("walls", boundary, &ClassicMD::Boundary::constrain);
    constraints.construct<Pipeline, PipelineAPI>();
    auto forces = step.branch("forces");
    forces.emplace<CallStage<ClassicMD::Boundary>, PipelineAPI>("walls", boundary, &ClassicMD::Boundary::addForces);
    forces.construct<Pipeline, PipelineAPI>();
    step.emplace<CallStage<Integrators::Verlet>, PipelineAPI>("correct", integrator, &Integrators::Verlet::correct);
    auto pipeline = step.construct<Pipeline, PipelineAPI>();

    // The branch object is the simulation itself, run as the branch service and shared
    // with readers (rendering) as a particle system with its boundaries.
    auto simulation = root.constructShared<ClassicMD::ClassicMD, ParticleDynamics::ParticleSystemAPI>(
        atoms, pipeline, commands, boundary);
    simulation.executeOn(commands);
    simulation.param("cell_size", &ClassicMD::ClassicMD::cellSize);
    simulation.action("step", &ClassicMD::ClassicMD::step);
    simulation.service(&ClassicMD::ClassicMD::run, &ClassicMD::ClassicMD::requestStop);
}

extern "C" void configure(NodeConfigure root) {
    Lattice::configurePipelines(root);
}
