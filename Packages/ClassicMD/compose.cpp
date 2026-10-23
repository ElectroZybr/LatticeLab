#include <Lattice/Composition.hpp>
#include <ClassicMD/AtomData.hpp>
#include <ClassicMD/AtomStorage.hpp>
#include <ClassicMD/ClassicMD.hpp>
#include <ClassicMD/SimulationLoop.hpp>
#include <Integrators/Verlet.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "ClassicMD",
    .version = {1, 0, 0},
};

// Default MD branch. Simulation, storage and integrator are ordinary classes;
// tree placement, exposed APIs, exports and the service live only here.
extern "C" void compose(NodeBuild root) {
    auto data = root.emplace<ClassicMD::AtomData, StdData::NamedSoA, Lattice::Table>("data");
    Lattice::exportTable(data);
    auto atoms = root.emplace<ClassicMD::AtomStorage,
        ParticleDynamics::ParticleStorage, Lattice::Table>("atoms");
    Lattice::exportTable(atoms);

    auto verlet = root.emplace<Integrators::Verlet, ParticleDynamics::IntegratorAPI>("integrator", atoms);
    verlet.param("dt", &Integrators::Verlet::dt);

    auto simulation = root.emplace<ClassicMD::ClassicMD>("simulation", atoms, verlet);
    simulation.param("cell_size", &ClassicMD::ClassicMD::cellSize);
    simulation.action("step", &ClassicMD::ClassicMD::step);

    auto loop = root.emplace<ClassicMD::SimulationLoop>("loop", simulation);
    loop.service(&ClassicMD::SimulationLoop::run, &ClassicMD::SimulationLoop::requestStop);
}
