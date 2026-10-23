#include <Lattice/Composition.hpp>
#include <Lattice/Kernel/Model.hpp>
#include "src/AtomData.hpp"
#include "src/AtomStorage.hpp"
#include "src/ClassicMD.hpp"
#include "Integrators/src/Verlet.hpp"

namespace {
// This is the runtime service boundary. Simulation and integrator code do not
// receive builders or retain runtime handles.
class SimulationService final : public Model {
public:
    explicit SimulationService(ClassicMD::ClassicMD& simulation) : simulation_(simulation) {}
    ~SimulationService() override { stop(); }

private:
    void run() override {
        while (!stopRequested()) {
            simulation_.step();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    ClassicMD::ClassicMD& simulation_;
};

SimulationService buildDefault(NodeBuild root) {
    root.add<ClassicMD::AtomData>();
    auto atoms = root.add<ClassicMD::AtomStorage>();
    auto verlet = root.emplace<Integrators::Verlet>("integrator", *atoms);
    root.dependsOn(verlet, atoms);
    auto simulation = root.emplace<ClassicMD::ClassicMD>("simulation", *atoms, *verlet);
    root.dependsOn(simulation, atoms);
    root.dependsOn(simulation, verlet);
    root.dependsOn(simulation);
    root.param("dt", verlet->dt);
    root.param("cell_size", simulation->cellSize);
    return SimulationService{*simulation};
}
}

extern "C" bool plugin_register(Lattice::Blueprints& types) {
    using namespace Lattice;
    BlueprintRegister::add<ClassicMD::AtomData, StdData::NamedSoA>(types);
    BlueprintRegister::add<ClassicMD::AtomStorage, ParticleDynamics::ParticleStorage>(types);
    BlueprintRegister::object<ClassicMD::ClassicMD>(types);
    const auto id = BlueprintRegister::factory<SimulationService, Model>(
        types, buildDefault, "ClassicMD::Default");
    // Service shutdown is explicit composition policy, not a convention imposed
    // on ordinary object implementations.
    static const BlueprintOps ops = [&] {
        auto result = *types.require(id).meta.ops;
        result.retire = [](void* ptr) { static_cast<SimulationService*>(ptr)->retire(); };
        result.readyToDestroy = [](const void* ptr) {
            return static_cast<const SimulationService*>(ptr)->readyToDestroy();
        };
        return result;
    }();
    types.require(id).meta.ops = &ops;
    return true;
}
