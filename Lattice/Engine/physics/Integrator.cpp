#include "Integrator.h"

#include <algorithm>
#include <cmath>

#include "Lattice/Engine/World.h"
#include "Lattice/Engine/physics/Atom/AtomStorage.h"
#include "Lattice/Engine/restrict.h"
#include "Lattice/Plugins/ClassicMD/ClassicMDPlugin.h"

namespace {
void ensureBuiltInPluginsRegistered() {
    static const bool registered = [] {
        registerClassicMDPlugin(globalIntegratorRegistry());
        return true;
    }();
    (void)registered;
}

void postProcessVelocities(AtomStorage& atomStorage, float maxSpeed) {
    const float maxSpeedSqr = maxSpeed * maxSpeed;
    float* RESTRICT vx = atomStorage.vxData();
    float* RESTRICT vy = atomStorage.vyData();
    float* RESTRICT vz = atomStorage.vzData();

    const size_t mobileCount = atomStorage.mobileCount();
#pragma GCC ivdep
    for (size_t atomIndex = 0; atomIndex < mobileCount; ++atomIndex) {
        float vxValue = vx[atomIndex];
        float vyValue = vy[atomIndex];
        float vzValue = vz[atomIndex];

        const float speedSqr = vxValue * vxValue + vyValue * vyValue + vzValue * vzValue;
        if (speedSqr > maxSpeedSqr) {
            const float scale = maxSpeed / std::sqrt(speedSqr);
            vxValue *= scale;
            vyValue *= scale;
            vzValue *= scale;
        }

        vx[atomIndex] = vxValue;
        vy[atomIndex] = vyValue;
        vz[atomIndex] = vzValue;
    }
}
} // namespace

Integrator::Integrator() {
    ensureBuiltInPluginsRegistered();
    setIntegrator("verlet");
}

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

void Integrator::setAndersenTemperature(float temperature) { andersenTemperature_ = std::max(0.0f, temperature); }

float Integrator::andersenTemperature() const { return andersenTemperature_; }

void Integrator::step(StepData& stepData) {
    if (!impl_) {
        return;
    }

    impl_->step(stepData);

    if (maxParticleSpeed_ > 0.0f) {
        postProcessVelocities(stepData.world.getAtomStorage(), maxParticleSpeed_);
    }
}
