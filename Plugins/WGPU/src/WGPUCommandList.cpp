#include "WGPUCommandList.hpp"

#include "Surface.hpp"
#include "WGPUSurface.hpp"

#include <Lattice/Kernel/Exception.hpp>

namespace WGPU {

CommandList::CommandList(WGPUDevice device, WGPUQueue queue)
    : device_(device), queue_(queue) {
    wgpuQueueAddRef(queue_);

    WGPUCommandEncoderDescriptor desc{};
    encoder_ = wgpuDeviceCreateCommandEncoder(device_, &desc);
    if (!encoder_) {
        wgpuQueueRelease(queue_);
        queue_ = nullptr;
        throw Lattice::Exception(tag, "failed to create encoder");
    }
}

CommandList::~CommandList() {
    renderPass_.end();

    if (commandBuffer_)
        wgpuCommandBufferRelease(commandBuffer_);
    if (encoder_)
        wgpuCommandEncoderRelease(encoder_);
    if (queue_)
        wgpuQueueRelease(queue_);
}

GPU::RenderPass& CommandList::beginRenderPass(
    GPU::Surface& surface,
    GPU::Color clear
) {
    if (submitted_ || commandBuffer_)
        throw Lattice::Exception(tag, "command list already finished");

    auto* native = dynamic_cast<Surface*>(&surface);
    if (!native || native->device() != device_ || !native->view())
        throw Lattice::Exception(tag, "expected acquired surface from the same WGPU device");

    renderPass_.begin(encoder_, *native, clear);
    return renderPass_;
}

void CommandList::submit() {
    if (renderPass_.active())
        throw Lattice::Exception(tag, "end the render pass before submitting");

    if (submitted_)
        throw Lattice::Exception(tag, "command list already submitted");

    if (!commandBuffer_)
        commandBuffer_ = wgpuCommandEncoderFinish(encoder_, nullptr);

    if (!commandBuffer_)
        throw Lattice::Exception(tag, "failed to finish encoder");

    wgpuQueueSubmit(queue_, 1, &commandBuffer_);
    submitted_ = true;
}

}
