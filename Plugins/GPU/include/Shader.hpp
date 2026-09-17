#pragma once

#include <string>

namespace GPU {

enum class ShaderStage {
    None,
    Vertex,
    Fragment,
    Compute
};

struct ShaderDesc {
    ShaderStage stage = ShaderStage::None;
    std::string source;
};

struct Shader {
    virtual ~Shader() = default;
};

}