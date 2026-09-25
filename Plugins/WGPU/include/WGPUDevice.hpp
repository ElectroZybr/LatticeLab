#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <webgpu/webgpu.h>

#include "Device.hpp"

class NodeBuild;

namespace WGPU {

class Device final : public GPU::Device {
public:
    explicit Device(NodeBuild node, const Desc& = {});
    ~Device() override;

    std::unique_ptr<GPU::CommandList> createCommandList() override;
    std::unique_ptr<GPU::BindingSet> createBindingSet(
        GPU::Pipeline& pipeline,
        uint32_t group,
        std::span<const GPU::Binding> bindings
    ) override;
    void writeBuffer(
        GPU::Buffer& buffer,
        uint64_t offset,
        std::span<const std::byte> data
    ) override;

    WGPUDevice native() const noexcept;
    WGPUAdapter adapter() const noexcept;
    WGPUInstance instance() const noexcept;

private:
    WGPUAdapter adapter_ = nullptr;
    WGPUInstance instance_ = nullptr;
    WGPUDevice device_ = nullptr;
    WGPUQueue queue_ = nullptr;
};

}
