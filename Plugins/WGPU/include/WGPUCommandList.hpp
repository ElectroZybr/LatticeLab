#pragma once

#include <webgpu/webgpu.h>

#include "CommandList.hpp"
#include "WGPURenderPass.hpp"

namespace WGPU {

class CommandList final : public GPU::CommandList {
    static constexpr std::string_view tag = "WGPU::CommandList";
public:
    CommandList(WGPUDevice, WGPUQueue);
    ~CommandList() override;

    GPU::RenderPass& beginRenderPass(
        GPU::Surface& surface,
        GPU::Color clear = {}
    ) override;

    void submit() override;

private:
    WGPUDevice device_ = nullptr;
    WGPUQueue queue_ = nullptr;
    WGPUCommandEncoder encoder_ = nullptr;
    WGPUCommandBuffer commandBuffer_ = nullptr;
    RenderPass renderPass_;
    bool submitted_ = false;
};

}
