#pragma once

#include <webgpu/webgpu.h>

#include "Shader.hpp"

class NodeBuild;

namespace WGPU {

class Shader final : public GPU::Shader {
public:
    explicit Shader(NodeBuild node, const Desc& desc);
    ~Shader() override;

    WGPUShaderModule native() const noexcept;
    WGPUDevice device() const noexcept;

private:
    WGPUDevice device_ = nullptr;
    WGPUShaderModule shader_ = nullptr;
};

}
