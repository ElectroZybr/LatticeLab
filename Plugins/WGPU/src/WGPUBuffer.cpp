#include "WGPUBuffer.hpp"
#include "WGPUDevice.hpp"
#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Kernel/NodeViews.hpp>

namespace WGPU {

Buffer::Buffer(NodeBuild node, const Desc& desc) {
    const auto device = node.ancestor<Device>();

    WGPUBufferDescriptor nativeDesc{};
    nativeDesc.size = desc.size;
    nativeDesc.usage = static_cast<WGPUBufferUsage>(desc.usage);
    nativeDesc.mappedAtCreation = false;

    buffer_ = wgpuDeviceCreateBuffer(device->native(), &nativeDesc);
    if (!buffer_)
        throw Lattice::Exception<Buffer>("failed to create buffer");

    size_ = desc.size;
}

Buffer::~Buffer() {
    if (buffer_)
        wgpuBufferRelease(buffer_);
}

WGPUBuffer Buffer::native() const noexcept {
    return buffer_;
}

uint64_t Buffer::size() const noexcept {
    return size_;
}

}
