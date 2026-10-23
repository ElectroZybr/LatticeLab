#include "Resources.hpp"

#include <string_view>
#include <vector>

#include <Lattice/Tools/Exception.hpp>
#include "Native.hpp"

namespace WGPU {

namespace {

WGPUStringView nativeString(std::string_view value) {
    return {value.data(), value.size()};
}

}

WGPUTextureFormat nativeFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8Unorm:             return WGPUTextureFormat_R8Unorm;
        case TextureFormat::RG8Unorm:            return WGPUTextureFormat_RG8Unorm;
        case TextureFormat::RGBA8Unorm:          return WGPUTextureFormat_RGBA8Unorm;
        case TextureFormat::BGRA8Unorm:          return WGPUTextureFormat_BGRA8Unorm;
        case TextureFormat::R16Float:            return WGPUTextureFormat_R16Float;
        case TextureFormat::RG16Float:           return WGPUTextureFormat_RG16Float;
        case TextureFormat::RGBA16Float:         return WGPUTextureFormat_RGBA16Float;
        case TextureFormat::R32Float:            return WGPUTextureFormat_R32Float;
        case TextureFormat::RG32Float:           return WGPUTextureFormat_RG32Float;
        case TextureFormat::RGBA32Float:         return WGPUTextureFormat_RGBA32Float;
        case TextureFormat::Depth16Unorm:        return WGPUTextureFormat_Depth16Unorm;
        case TextureFormat::Depth24Plus:         return WGPUTextureFormat_Depth24Plus;
        case TextureFormat::Depth24PlusStencil8: return WGPUTextureFormat_Depth24PlusStencil8;
        case TextureFormat::Depth32Float:        return WGPUTextureFormat_Depth32Float;
        default:                                 return WGPUTextureFormat_Undefined;
    }
}

Buffer::Buffer(Device& device, const BufferDesc& desc)
    : Tracked(device, "Buffer", desc.label, desc.size), size_(desc.size) {
    WGPUBufferDescriptor nativeDesc{};
    nativeDesc.label = nativeString(desc.label);
    nativeDesc.size = desc.size;
    nativeDesc.usage = static_cast<WGPUBufferUsage>(desc.usage);
    nativeDesc.mappedAtCreation = false;

    buffer_ = wgpuDeviceCreateBuffer(device.native(), &nativeDesc);
    if (!buffer_)
        throw Lattice::Exception<Buffer>("failed to create buffer '{}'", desc.label);
}

Buffer::~Buffer() {
    if (buffer_)
        wgpuBufferRelease(buffer_);
}

Texture::Texture(Device& device, const TextureDesc& desc)
    : Tracked(device, "Texture", desc.label, 0) {
    WGPUTextureDescriptor nativeDesc{};
    nativeDesc.label = nativeString(desc.label);
    nativeDesc.size = {desc.width, desc.height, desc.depth};
    nativeDesc.mipLevelCount = desc.mipLevels;
    nativeDesc.sampleCount = 1;
    nativeDesc.dimension = WGPUTextureDimension_2D;
    nativeDesc.format = nativeFormat(desc.format);
    nativeDesc.usage = static_cast<WGPUTextureUsage>(desc.usage);

    texture_ = wgpuDeviceCreateTexture(device.native(), &nativeDesc);
    if (!texture_)
        throw Lattice::Exception<Texture>("failed to create texture '{}'", desc.label);
    view_ = wgpuTextureCreateView(texture_, nullptr);
    if (!view_) {
        wgpuTextureRelease(texture_);
        throw Lattice::Exception<Texture>("failed to create view of texture '{}'", desc.label);
    }
    width_ = desc.width;
    height_ = desc.height;
    format_ = desc.format;
}

Texture::~Texture() {
    if (view_)
        wgpuTextureViewRelease(view_);
    if (texture_)
        wgpuTextureRelease(texture_);
}

Shader::Shader(Device& device, const ShaderDesc& desc)
    : Tracked(device, "Shader", desc.label, 0) {
    WGPUShaderSourceWGSL source{};
    source.chain.sType = WGPUSType_ShaderSourceWGSL;
    source.code = nativeString(desc.source);

    WGPUShaderModuleDescriptor nativeDesc{};
    nativeDesc.nextInChain = &source.chain;
    nativeDesc.label = nativeString(desc.label);

    shader_ = wgpuDeviceCreateShaderModule(device.native(), &nativeDesc);
    if (!shader_)
        throw Lattice::Exception<Shader>("failed to create shader '{}'", desc.label);
}

Shader::~Shader() {
    if (shader_)
        wgpuShaderModuleRelease(shader_);
}

Pipeline::Pipeline(Device& device, Shader& shader, const PipelineDesc& desc)
    : Tracked(device, "Pipeline", desc.label, 0) {
    WGPUColorTargetState color{};
    color.format = nativeFormat(desc.colorFormat);
    color.writeMask = WGPUColorWriteMask_All;

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
    pipeline.multisample.count = 1;
    pipeline.multisample.mask = 0xFFFFFFFF;

    WGPUDepthStencilState depth{};
    if (desc.depthFormat != TextureFormat::Undefined) {
        depth.format = nativeFormat(desc.depthFormat);
        depth.depthWriteEnabled = desc.depthWrite ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        depth.depthCompare = WGPUCompareFunction_Less;
        depth.stencilFront.compare = WGPUCompareFunction_Always;
        depth.stencilBack.compare = WGPUCompareFunction_Always;
        depth.stencilReadMask = 0;
        depth.stencilWriteMask = 0;
        pipeline.depthStencil = &depth;
    }

    WGPUPipelineLayout layout = nullptr;
    if (!desc.layouts.empty()) {
        std::vector<WGPUBindGroupLayout> groups;
        for (const BindingLayout* group : desc.layouts) {
            if (!group)
                throw Lattice::Exception<Pipeline>("pipeline '{}': layout of group {} is null", desc.label, groups.size());
            groups.push_back(group->native());
        }
        WGPUPipelineLayoutDescriptor layoutDesc{};
        layoutDesc.label = nativeString(desc.label);
        layoutDesc.bindGroupLayoutCount = groups.size();
        layoutDesc.bindGroupLayouts = groups.data();
        layout = wgpuDeviceCreatePipelineLayout(device.native(), &layoutDesc);
        if (!layout)
            throw Lattice::Exception<Pipeline>("failed to create layout of pipeline '{}'", desc.label);
        pipeline.layout = layout;
    }

    pipeline_ = wgpuDeviceCreateRenderPipeline(device.native(), &pipeline);
    if (layout)
        wgpuPipelineLayoutRelease(layout);
    if (!pipeline_)
        throw Lattice::Exception<Pipeline>("failed to create pipeline '{}'", desc.label);
}

namespace {

WGPUBufferBindingType nativeBindingType(BindingType type) {
    switch (type) {
        case BindingType::Uniform:         return WGPUBufferBindingType_Uniform;
        case BindingType::Storage:         return WGPUBufferBindingType_Storage;
        case BindingType::ReadOnlyStorage: return WGPUBufferBindingType_ReadOnlyStorage;
    }
    return WGPUBufferBindingType_Undefined;
}

}

BindingLayout::BindingLayout(Device& device, std::span<const LayoutEntry> entries, std::string label)
    : Tracked(device, "BindingLayout", label, 0) {
    std::vector<WGPUBindGroupLayoutEntry> native(entries.size());
    for (size_t i = 0; i < entries.size(); ++i) {
        native[i].binding = entries[i].binding;
        native[i].visibility = static_cast<WGPUShaderStage>(entries[i].visibility);
        native[i].buffer.type = nativeBindingType(entries[i].type);
    }
    WGPUBindGroupLayoutDescriptor desc{};
    desc.label = nativeString(label);
    desc.entryCount = native.size();
    desc.entries = native.data();
    layout_ = wgpuDeviceCreateBindGroupLayout(device.native(), &desc);
    if (!layout_)
        throw Lattice::Exception<BindingLayout>("failed to create bind group layout '{}'", label);
}

BindingLayout::~BindingLayout() {
    if (layout_)
        wgpuBindGroupLayoutRelease(layout_);
}

Pipeline::~Pipeline() {
    if (pipeline_)
        wgpuRenderPipelineRelease(pipeline_);
}

BindingSet::BindingSet(Device& device, Pipeline& pipeline, uint32_t group, std::span<const Binding> bindings)
    : Tracked(device, "BindingSet", {}, 0) {
    auto layout = wgpuRenderPipelineGetBindGroupLayout(pipeline.native(), group);
    if (!layout)
        throw Lattice::Exception<BindingSet>("failed to get bind group layout");
    try {
        create(layout, bindings);
    } catch (...) {
        wgpuBindGroupLayoutRelease(layout);
        throw;
    }
    wgpuBindGroupLayoutRelease(layout);
}

BindingSet::BindingSet(Device& device, BindingLayout& layout, std::span<const Binding> bindings)
    : Tracked(device, "BindingSet", {}, 0) {
    create(layout.native(), bindings);
}

void BindingSet::create(WGPUBindGroupLayout layout, std::span<const Binding> bindings) {
    std::vector<WGPUBindGroupEntry> entries;
    entries.reserve(bindings.size());
    for (const auto& binding : bindings) {
        if (!binding.buffer)
            throw Lattice::Exception<BindingSet>("binding {} has no buffer", binding.binding);
        WGPUBindGroupEntry entry{};
        entry.binding = binding.binding;
        entry.buffer = binding.buffer->native();
        entry.offset = binding.offset;
        entry.size = binding.size ? binding.size : binding.buffer->size();
        entries.push_back(entry);
    }

    WGPUBindGroupDescriptor desc{};
    desc.layout = layout;
    desc.entryCount = entries.size();
    desc.entries = entries.data();

    bindGroup_ = wgpuDeviceCreateBindGroup(device().native(), &desc);
    if (!bindGroup_)
        throw Lattice::Exception<BindingSet>("failed to create bind group");
}

BindingSet::~BindingSet() {
    if (bindGroup_)
        wgpuBindGroupRelease(bindGroup_);
}

}
