#include "CommandList.hpp"

#include <Lattice/Tools/Exception.hpp>

namespace Wgpu {

using Error = Lattice::Exception<CommandList>;

CommandList::CommandList(Device& device) : device_(device) {
    WGPUCommandEncoderDescriptor desc{};
    encoder_ = wgpuDeviceCreateCommandEncoder(device_.native(), &desc);
    if (!encoder_)
        throw Error("failed to create encoder");
}

CommandList::~CommandList() {
    if (commandBuffer_)
        wgpuCommandBufferRelease(commandBuffer_);
    if (encoder_)
        wgpuCommandEncoderRelease(encoder_);
}

void CommandList::requireRecording() const {
    if (submitted_ || commandBuffer_)
        throw Error("command list already finished");
}

WGPUCommandEncoder CommandList::encoder() {
    requireRecording();
    return encoder_;
}

void CommandList::copyBuffer(Buffer& source, uint64_t sourceOffset, Buffer& target, uint64_t targetOffset,
                             uint64_t size) {
    requireRecording();
    if (openPasses_)
        throw Error("end the pass before copying");
    wgpuCommandEncoderCopyBufferToBuffer(encoder_, source.native(), sourceOffset, target.native(), targetOffset, size);
}

void CommandList::submit() {
    if (openPasses_)
        throw Error("end the pass before submitting");
    if (submitted_)
        throw Error("command list already submitted");
    if (!commandBuffer_)
        commandBuffer_ = wgpuCommandEncoderFinish(encoder_, nullptr);
    if (!commandBuffer_)
        throw Error("failed to finish encoder");
    wgpuQueueSubmit(device_.queue(), 1, &commandBuffer_);
    submitted_ = true;
}

}
