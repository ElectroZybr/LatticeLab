#include "CommandList.hpp"

#include <Lattice/tools/Exception.hpp>

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

void CommandList::copyTexture(Texture& source, Buffer& target, uint32_t bytesPerRow) {
    requireRecording();
    if (openPasses_)
        throw Error("end the pass before copying");
    if (bytesPerRow % RowAlignment)
        throw Error("bytes per row {} are not a multiple of {}", bytesPerRow, RowAlignment);
    if (uint64_t(bytesPerRow) * source.height() > target.size())
        throw Error("{} rows of {} bytes do not fit a buffer of {}", source.height(), bytesPerRow, target.size());
    WGPUTexelCopyTextureInfo from{};
    from.texture = source.native();
    from.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyBufferInfo to{};
    to.buffer = target.native();
    to.layout.bytesPerRow = bytesPerRow;
    to.layout.rowsPerImage = source.height();
    const WGPUExtent3D extent{source.width(), source.height(), 1};
    wgpuCommandEncoderCopyTextureToBuffer(encoder_, &from, &to, &extent);
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
