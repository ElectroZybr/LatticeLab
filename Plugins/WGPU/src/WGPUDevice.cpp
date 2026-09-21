#include "WGPUDevice.hpp"
#include "WGPU.hpp"
#include "RenderPass.hpp"
#include "Surface.hpp"

#include <Lattice/Kernel/Exception.hpp>

namespace WGPU {

Device::Device(Lattice::Node& node, const Desc&) {
    auto backend = node.requireParent<WGPU>();
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

std::unique_ptr<GPU::CommandList> Device::createCommandList() {
    return std::make_unique<CommandList>(device_, queue_);
}

std::unique_ptr<GPU::BindingSet> Device::createBindingSet(
    GPU::Pipeline& pipeline, uint32_t group, std::span<const GPU::Binding> bindings)
{
    auto* nativePipeline = dynamic_cast<Pipeline*>(&pipeline);
    if (!nativePipeline)
        throw Lattice::Exception("WGPU::Device", "expected WGPU pipeline");

    return std::make_unique<BindingSet>(device_, *nativePipeline, group, bindings);
}

}
