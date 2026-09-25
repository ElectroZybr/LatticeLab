#include "WGPUShader.hpp"

#include "WGPUDevice.hpp"

#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/NodeViews.hpp>

namespace WGPU {

Shader::Shader(NodeBuild node, const Desc& desc) {
    const auto device = node.ancestor<Device>();
    device_ = device->native();

    if (desc.language != GPU::ShaderLanguage::WGSL)
        throw Lattice::Exception("WGPU::Shader", "unsupported shader language");

    WGPUShaderSourceWGSL source{};
    source.chain.sType = WGPUSType_ShaderSourceWGSL;
    source.code = {desc.source.data(), desc.source.size()};

    WGPUShaderModuleDescriptor nativeDesc{};
    nativeDesc.nextInChain = &source.chain;

    const auto name = node.name();
    nativeDesc.label = {name.data(), name.size()};

    shader_ = wgpuDeviceCreateShaderModule(device_, &nativeDesc);
    if (!shader_)
        throw Lattice::Exception("WGPU::Shader", "failed to create shader");
}

Shader::~Shader() {
    if (shader_)
        wgpuShaderModuleRelease(shader_);
}

WGPUShaderModule Shader::native() const noexcept {
    return shader_;
}

WGPUDevice Shader::device() const noexcept {
    return device_;
}

}
