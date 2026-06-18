#include "Integrator.h"

#include <algorithm>

#include "Lattice/Plugins/ClassicMD/Integrators/StepOps.h"

Integrator::Integrator() {
    setIntegrator("verlet");
}

Integrator::~Integrator() = default;
Integrator::Integrator(Integrator&&) noexcept = default;
Integrator& Integrator::operator=(Integrator&&) noexcept = default;

bool Integrator::setIntegrator(std::string_view id) {
    const IntegratorMeta* meta = globalIntegratorRegistry().find(id);
    if (meta == nullptr || meta->factory == nullptr) {
        return false;
    }

    std::unique_ptr<IIntegrator> impl = meta->factory();
    if (!impl) {
        return false;
    }

    currentId_ = meta->id;
    impl_ = std::move(impl);
    return true;
}

void Integrator::setMaxParticleSpeed(float maxSpeed) { maxParticleSpeed_ = std::max(0.0f, maxSpeed); }

void Integrator::setAccelDamping(float accelDamping) { accelDamping_ = std::clamp(accelDamping, 0.0f, 1.0f); }

void Integrator::step(StepContext& stepContext) {
    if (!impl_) {
        return;
    }

    impl_->step(stepContext);

    if (maxParticleSpeed_ > 0.0f) {
        StepOps::postProcessVelocities(stepContext.world.getAtomStorage(), maxParticleSpeed_);
    }
}
