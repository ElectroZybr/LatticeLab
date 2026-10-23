#include <Lattice/Composition.hpp>
#include <ClassicMD/AtomData.hpp>
#include <ClassicMD/AtomStorage.hpp>
#include <ClassicMD/ClassicMD.hpp>
#include <Integrators/Verlet.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "ClassicMD",
    .version = {1, 0, 0},
};

extern "C" void compose(NodeBuild root) {
    auto data = root.emplace<ClassicMD::AtomData, StdData::NamedSoA, Lattice::Table>("data");
    Lattice::exportTable(data);
    auto atoms = root.emplace<ClassicMD::AtomStorage, ParticleDynamics::ParticleStorage, Lattice::Table>("atoms");
    Lattice::exportTable(atoms);

    auto verlet = root.emplace<Integrators::Verlet, ParticleDynamics::IntegratorAPI>("integrator", atoms);
    verlet.param("dt", &Integrators::Verlet::dt);

    // The branch object is the simulation itself, run as the branch service.
    auto simulation = root.construct<ClassicMD::ClassicMD>(atoms, verlet);
    simulation.param("cell_size", &ClassicMD::ClassicMD::cellSize);
    simulation.action("step", &ClassicMD::ClassicMD::step);
    simulation.service(&ClassicMD::ClassicMD::run, &ClassicMD::ClassicMD::requestStop);
}
