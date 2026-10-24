#include "ComputePass.hpp"

#include <Lattice/Tools/Exception.hpp>

#include "Native.hpp"
#include "Timer.hpp"

namespace Wgpu {

using Error = Lattice::Exception<ComputePass>;

ComputePass::ComputePass(CommandList& commands, std::string_view label, Timer* timer) : commands_(commands) {
    WGPUComputePassDescriptor desc{};
    desc.label = nativeString(label);
    if (timer)
        desc.timestampWrites = timer->pass(label);
    pass_ = wgpuCommandEncoderBeginComputePass(commands.encoder(), &desc);
    if (!pass_)
        throw Error("failed to begin compute pass");
    commands_.passBegan();
}

ComputePass::~ComputePass() {
    end();
}

void ComputePass::requireActive() const {
    if (!pass_)
        throw Error("no active compute pass");
}

void ComputePass::setPipeline(ComputePipeline& pipeline) {
    requireActive();
    if (&pipeline.device() != &commands_.device())
        throw Error("expected a pipeline of the same device");
    wgpuComputePassEncoderSetPipeline(pass_, pipeline.native());
    pipeline_ = true;
}

void ComputePass::setBindings(uint32_t group, BindingSet& bindings) {
    requireActive();
    wgpuComputePassEncoderSetBindGroup(pass_, group, bindings.native(), 0, nullptr);
}

void ComputePass::dispatch(uint32_t x, uint32_t y, uint32_t z) {
    requireActive();
    if (!pipeline_)
        throw Error("dispatch requires a pipeline");
    wgpuComputePassEncoderDispatchWorkgroups(pass_, x, y, z);
}

void ComputePass::end() {
    if (!pass_)
        return;
    wgpuComputePassEncoderEnd(pass_);
    wgpuComputePassEncoderRelease(pass_);
    pass_ = nullptr;
    commands_.passEnded();
}

}
