#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>

#include "Lattice/Engine/StepContext.h"
#include "Lattice/Engine/physics/Registrator.hpp"

class AtomStorage;
class ForceField;
class NeighborList;
class World;

/// абстрактный интегратор
class IIntegrator : public IModule {
public:
    virtual ~IIntegrator() = default;
    virtual void step(StepContext& stepContext) = 0;
};

class Integrator : public RegisteredModuleOwner<Integrator, IIntegrator> {
public:
    Integrator() { setIntegrator("verlet"); }
    Integrator(const Integrator&) = delete;
    Integrator& operator=(const Integrator&) = delete;
    Integrator(Integrator&&) noexcept = default;
    Integrator& operator=(Integrator&&) noexcept = default;

    static ModuleRegistry<IIntegrator>& registry() {
        static ModuleRegistry<IIntegrator> registry;
        return registry;
    }

    bool setIntegrator(std::string_view id) { return set(id); }
    std::string_view getIntegrator() const { return get(); }
    void setMaxParticleSpeed(float maxSpeed) { maxParticleSpeed_ = std::max(0.0f, maxSpeed); }
    float maxParticleSpeed() const { return maxParticleSpeed_; }
    void step(StepContext& stepContext) {
        if (!impl_) {
            return;
        }
        impl_->step(stepContext);
    }
private:
    float maxParticleSpeed_ = 0.0f;
};

#define REGISTER_INTEGRATOR(Type) REGISTER_MODULE(Type, IIntegrator, Integrator::registry, autoRegIntegrator_)
