#include "WGPU.hpp"
#include "WGPUDevice.hpp"
#include "WGPUCommandList.hpp"
#include "WGPURenderPass.hpp"
#include "WGPUBuffer.hpp"
#include "WGPUSurface.hpp"

#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/NodeViews.hpp>

namespace WGPU {

Device::Device(NodeBuild node, const Desc&) {
    auto backend = node.ancestor<WGPU>();
    device_ = backend->createDevice(adapter_);
    instance_ = backend->native();
    wgpuInstanceAddRef(instance_);
    queue_ = wgpuDeviceGetQueue(device_);
}

Device::~Device() {
    if (queue_)
        wgpuQueueRelease(queue_);
    if (device_)
        wgpuDeviceRelease(device_);
    if (adapter_)
        wgpuAdapterRelease(adapter_);
    if (instance_)
        wgpuInstanceRelease(instance_);
}

WGPUDevice Device::native() const noexcept {
    return device_;
}

WGPUAdapter Device::adapter() const noexcept {
    return adapter_;
}

WGPUInstance Device::instance() const noexcept {
    return instance_;
}

std::unique_ptr<GPU::CommandList> Device::createCommandList() {
    return std::make_unique<CommandList>(device_, queue_);
}

std::unique_ptr<GPU::BindingSet> Device::createBindingSet(
    GPU::Pipeline& pipeline,
    uint32_t group,
    std::span<const GPU::Binding> bindings
) {
    auto* native = dynamic_cast<Pipeline*>(&pipeline);
    if (!native)
        throw Lattice::Exception("WGPU::Device", "expected WGPU pipeline");

    return std::make_unique<BindingSet>(device_, *native, group, bindings);
}

void Device::writeBuffer(GPU::Buffer& buffer, uint64_t offset, std::span<const std::byte> data) {
    auto* native = dynamic_cast<Buffer*>(&buffer);
    if (!native)
        throw Lattice::Exception("WGPU::Device", "expected WGPU buffer");

    wgpuQueueWriteBuffer(queue_, native->native(), offset, data.data(), data.size());
}

}
