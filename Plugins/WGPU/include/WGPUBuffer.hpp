#pragma once

#include "Buffer.hpp"
#include "WGPUDevice.hpp"
#include <webgpu/webgpu.h>

#include "Lattice/Kernel/Node.hpp"


namespace WGPU {
// ---------- resources ----------
class Buffer final : public GPU::Buffer {
public:
    explicit Buffer(Lattice::Node& node, const Desc& desc) {
        const auto device = node.requireParent<Device>();

        WGPUBufferDescriptor nativeDesc = {};
        auto name = node.name();
        nativeDesc.label = WGPUStringView{name.data(), name.size()};
        nativeDesc.size = desc.size;
        nativeDesc.usage = static_cast<WGPUBufferUsage>(desc.usage);
        nativeDesc.mappedAtCreation = false;

        buffer_ = wgpuDeviceCreateBuffer(device->native(), &nativeDesc);

        if (!buffer_)
            throw Lattice::Exception("WGPU::Buffer", "failed to create buffer");
    }

    ~Buffer() override {
        if (buffer_)
            wgpuBufferRelease(buffer_);
    }

    WGPUBuffer native() const noexcept { return buffer_; }
    uint64_t size() const noexcept { return size_; }

private:
    WGPUBuffer buffer_ = nullptr;
    uint64_t size_ = 0;
};

}