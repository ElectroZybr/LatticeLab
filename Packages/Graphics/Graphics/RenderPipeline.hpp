#pragma once

#include <webgpu/webgpu.h>
#include <Wgpu/Device.hpp>
#include <Wgpu/Resources.hpp>

#include "RenderTypes.hpp"

/// A render pipeline on explicit bind group layouts (triangle list, one color target).
class RenderPipeline final : Wgpu::Tracked {
public:
    RenderPipeline(Wgpu::Device& device, Wgpu::Shader& shader, const RenderPipelineDesc& desc);
    ~RenderPipeline();

    WGPURenderPipeline native() const noexcept { return pipeline_; }
    Wgpu::Device& device() const noexcept { return Tracked::device(); }

private:
    WGPURenderPipeline pipeline_ = nullptr;
};
