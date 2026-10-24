#include <Lattice/Composition.hpp>
#include <ClassicMD/AtomData.hpp>
#include <ClassicMD/AtomStorage.hpp>
#include <ClassicMD/ClassicMD.hpp>
#include <Integrators/Verlet.hpp>
#include <Tasks/Queue.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "ClassicMD",
    .version = {1, 0, 0},
};

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

    auto verlet = root.emplace<Integrators::Verlet, ParticleDynamics::IntegratorAPI>("integrator", atoms);
    verlet.executeOn(commands);
    verlet.param("dt", &Integrators::Verlet::dt);

    // The branch object is the simulation itself, run as the branch service and shared
    // with readers (rendering) as a particle system.
    auto simulation = root.constructShared<ClassicMD::ClassicMD, ParticleDynamics::ParticleSystemAPI>(
        atoms, verlet, commands);
    simulation.executeOn(commands);
    simulation.param("cell_size", &ClassicMD::ClassicMD::cellSize);
    simulation.action("step", &ClassicMD::ClassicMD::step);
    simulation.service(&ClassicMD::ClassicMD::run, &ClassicMD::ClassicMD::requestStop);
}
