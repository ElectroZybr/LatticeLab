#pragma once

#include <glm/vec3.hpp>

// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include <Lattice/Kernel/ServiceAPI.hpp>
#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Kernel/Model.hpp>

// Plugin dependences
#include <ParticleDynamics/include/ParticleAPI.hpp>
#include <ParticleDynamics/include/ParticleStorage.hpp>

// Source
#include "AtomData.hpp"
#include "AtomStorage.hpp"

namespace ClassicMD {

class ClassicMD final : public Model {
public:
    explicit ClassicMD(Lattice::NodeBuildView& universe) {
        //universe.makeFocusScope();
        atomData    = universe.add<AtomData>();//.focus();
        atoms       = universe.add<AtomStorage>();//.focus();
        spatialGrid = universe.slot<ParticleDynamics::SpatialIndexAPI>();
        integrator  = universe.slot<ParticleDynamics::IntegratorAPI>();
    }

    void configure(Lattice::Node& universe) {
        // integrator.use<Integrators::Verlet>();
    //     universe.on("CreateVerlet", [this]() { integrator.use("Verlet"); });
    //     universe.on("selectUniverse", [&universe] { universe.requireContext().activateFocus(universe.getFocusScopeId()); });
    }

    void run() override {
        atoms->add({0, 0, 0}, {1, 10, 0});
        while (!stopRequested()) {
            if (integrator)
                integrator->step();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    ~ClassicMD() {
        Logger::info("ClassicMD", "destroying object");
    }

private:
    Ref<AtomData> atomData;
    Ref<AtomStorage> atoms;
    Slot<ParticleDynamics::IntegratorAPI> integrator;
    Slot<ParticleDynamics::SpatialIndexAPI> spatialGrid;
};

} // namespace ClassicMD
