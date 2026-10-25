#include "GpuBoundaries.hpp"

GpuBoundaries::GpuBoundaries(Wgpu::Device& device, ParticleDynamics::ParticleSystemAPI& system)
    : device_(device), system_(&system), attached_(true) {
    update();
}

GpuBoundaries::~GpuBoundaries() = default;

void GpuBoundaries::set(const Boundaries::Space& space, Boundaries::Program program) {
    space_.emplace(space);
    if (geometry_)
        geometry_->upload(std::move(program));
    else
        geometry_ = std::make_unique<Boundaries::Wgpu::GpuGeometry>(device_, std::move(program));
    ++version_;
}

void GpuBoundaries::update() {
    std::optional<Copy> copy;
    {
        std::lock_guard lock(mutex_);
        copy.swap(arrived_);
    }
    if (copy) {
        copied_ = copy->version;
        set(copy->space, std::move(copy->program));
    }
    if (!attached_ || pending_.exchange(true))
        return;
    system_->commands().post([this, seen = copied_] {
        const ParticleDynamics::BoundaryView view = system_->boundaries();
        if (view.space && view.program && view.version != seen) {
            std::lock_guard lock(mutex_);
            arrived_ = Copy{*view.space, *view.program, view.version};
        }
        pending_ = false;
    });
}
