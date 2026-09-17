#pragma once

#include <Lattice/Kernel/Component.hpp>

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

struct Shader : public Lattice::Component {
    virtual ~Shader() = default;
};

}