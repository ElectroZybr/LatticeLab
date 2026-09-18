#pragma once
#include <GPU/include/Shader.hpp>
#include <GPU/include/Texture.hpp>

namespace Graphics {
struct RenderPipelineDesc {
    GPU::Shader* shader = nullptr;
    GPU::TextureFormat colorFormat = GPU::TextureFormat::BGRA8Unorm;
    std::string vertexEntry = "vs";
    std::string fragmentEntry = "fs";
};

class RenderPipeline : public Lattice::Component {
public:
    using Desc = RenderPipelineDesc;
    virtual ~RenderPipeline() = default;
};

struct ClearColor { double r = 0.1, g = 0.2, b = 0.3, a = 1.0; };
}
