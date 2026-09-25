#pragma once

#include <webgpu/webgpu.h>

#include "Texture.hpp"

class NodeBuild;

namespace WGPU {

class Texture final : public GPU::Texture {
public:
    explicit Texture(NodeBuild node, const Desc& desc);
    ~Texture() override;

    WGPUTexture native() const noexcept;

private:
    WGPUTexture texture_ = nullptr;
};

}
