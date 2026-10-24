#include "RenderPipeline.hpp"

#include <vector>

#include <Lattice/Tools/Exception.hpp>
#include <Wgpu/Native.hpp>

using Error = Lattice::Exception<RenderPipeline>;
using Wgpu::nativeFormat;
using Wgpu::nativeString;

RenderPipeline::RenderPipeline(Wgpu::Device& device, Wgpu::Shader& shader, const RenderPipelineDesc& desc)
    : Tracked(device, "Pipeline", desc.label, 0) {
    if (desc.layouts.empty())
        throw Error("pipeline '{}' needs explicit bind group layouts", desc.label);

    WGPUColorTargetState color{};
    color.format = nativeFormat(desc.colorFormat);
    color.writeMask = WGPUColorWriteMask_All;
    WGPUBlendState blend{};
    if (desc.blend) {
        blend.color = {.operation = WGPUBlendOperation_Add, .srcFactor = WGPUBlendFactor_SrcAlpha,
                       .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha};
        blend.alpha = {.operation = WGPUBlendOperation_Add, .srcFactor = WGPUBlendFactor_One,
                       .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha};
        color.blend = &blend;
    }

    WGPUFragmentState fragment{};
    fragment.module = shader.native();
    fragment.entryPoint = nativeString(desc.fragmentEntry);
    fragment.targetCount = 1;
    fragment.targets = &color;

    WGPURenderPipelineDescriptor pipeline{};
    pipeline.label = nativeString(desc.label);
    pipeline.vertex.module = shader.native();
    pipeline.vertex.entryPoint = nativeString(desc.vertexEntry);
    pipeline.fragment = &fragment;
    pipeline.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pipeline.primitive.frontFace = WGPUFrontFace_CCW;
    pipeline.primitive.cullMode = desc.cull == CullMode::Back ? WGPUCullMode_Back : WGPUCullMode_None;
    pipeline.multisample.count = 1;
    pipeline.multisample.mask = 0xFFFFFFFF;

    WGPUDepthStencilState depth{};
    if (desc.depthFormat != Wgpu::TextureFormat::Undefined) {
        depth.format = nativeFormat(desc.depthFormat);
        depth.depthWriteEnabled = desc.depthWrite ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        switch (desc.depthCompare) {
        case DepthCompare::Less: depth.depthCompare = WGPUCompareFunction_Less; break;
        case DepthCompare::LessEqual: depth.depthCompare = WGPUCompareFunction_LessEqual; break;
        case DepthCompare::Greater: depth.depthCompare = WGPUCompareFunction_Greater; break;
        case DepthCompare::Always: depth.depthCompare = WGPUCompareFunction_Always; break;
        }
        depth.stencilFront.compare = WGPUCompareFunction_Always;
        depth.stencilBack.compare = WGPUCompareFunction_Always;
        depth.stencilReadMask = 0;
        depth.stencilWriteMask = 0;
        pipeline.depthStencil = &depth;
    }

    std::vector<WGPUBindGroupLayout> groups;
    for (const Wgpu::BindingLayout* group : desc.layouts) {
        if (!group)
            throw Error("pipeline '{}': layout of group {} is null", desc.label, groups.size());
        groups.push_back(group->native());
    }
    WGPUPipelineLayoutDescriptor layoutDesc{};
    layoutDesc.label = nativeString(desc.label);
    layoutDesc.bindGroupLayoutCount = groups.size();
    layoutDesc.bindGroupLayouts = groups.data();
    WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(device.native(), &layoutDesc);
    if (!layout)
        throw Error("failed to create layout of pipeline '{}'", desc.label);
    pipeline.layout = layout;

    pipeline_ = wgpuDeviceCreateRenderPipeline(device.native(), &pipeline);
    wgpuPipelineLayoutRelease(layout);
    if (!pipeline_)
        throw Error("failed to create pipeline '{}'", desc.label);
}

RenderPipeline::~RenderPipeline() {
    if (pipeline_)
        wgpuRenderPipelineRelease(pipeline_);
}
