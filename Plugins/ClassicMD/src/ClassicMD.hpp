#pragma once
#include <Lattice/Kernel/Model.hpp>
#include <Lattice/Kernel/NodeViews.hpp>
#include <ParticleDynamics/include/ParticleAPI.hpp>
#include <glm/ext/vector_float3.hpp>
#include "Integrators/src/Verlet.hpp"
#include "AtomData.hpp"
#include "AtomStorage.hpp"

namespace ClassicMD {
class ClassicMD final : public Model {
public:
    explicit ClassicMD(NodeBuild universe)
        : atomData(universe.add<AtomData>())
        , atoms(universe.add<AtomStorage>())
        , integrator(universe.addSlot<ParticleDynamics::IntegratorAPI>())
        , spatialGrid(universe.addSlot<ParticleDynamics::SpatialIndexAPI>()) {
            universe.param("dt", dt);
            universe.param("cell_size", cellSize);
        }

    void configure(NodeConfigure universe) {
        integrator.choice<Integrators::Verlet>();
    }

    void run() override {
        atoms->add({0, 0, 0}, {1, 10, 0});
        Logger::info("ClassicMD", "Particle added; count: {}", atoms->size());
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
    float dt = 0;
    glm::vec3 cellSize{};
    
    Ref<AtomData> atomData;
    Ref<AtomStorage> atoms;
    Slot<ParticleDynamics::IntegratorAPI> integrator;
    Slot<ParticleDynamics::SpatialIndexAPI> spatialGrid;
};

} // namespace ClassicMD
