#pragma once

#include "Device.hpp"
#include <webgpu/webgpu.h>
#include <memory>
#include <span>

#include "Lattice/Kernel/Node.hpp"

namespace WGPU {

class Device final : public GPU::Device {
public:
    explicit Device(Lattice::Node& node, const Desc& = {});
    ~Device() override;

    std::unique_ptr<GPU::CommandList> createCommandList() override;
    std::unique_ptr<GPU::BindingSet> createBindingSet(GPU::Pipeline& pipeline, uint32_t group, std::span<const GPU::Binding> bindings) override;
    void writeBuffer(GPU::Buffer& buffer, uint64_t offset, std::span<const std::byte> data) override;

    WGPUDevice native() const noexcept { return device_; }
    WGPUAdapter adapter() const noexcept { return adapter_; }
    WGPUInstance instance() const noexcept { return instance_; }

private:
    WGPUAdapter adapter_ = nullptr;
    WGPUInstance instance_ = nullptr;
    WGPUDevice device_ = nullptr;
    WGPUQueue queue_ = nullptr;
};

}
