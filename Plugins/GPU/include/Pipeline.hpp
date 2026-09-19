#pragma once
#include <GPU/include/Shader.hpp>
#include <GPU/include/Texture.hpp>

namespace GPU {
struct PipelineDesc {
    GPU::Shader* shader = nullptr;
    GPU::TextureFormat colorFormat = GPU::TextureFormat::BGRA8Unorm;
    std::string vertexEntry = "vs";
    std::string fragmentEntry = "fs";
};

class Pipeline : public Lattice::Component {
public:
    using Desc = PipelineDesc;
    virtual ~Pipeline() = default;
};

}
