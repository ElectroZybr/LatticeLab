#pragma once

#include <string>
#include <Lattice/Kernel/Component.hpp>

namespace GPU {

enum class ShaderLanguage {
    WGSL
};

struct ShaderDesc {
    ShaderLanguage language = ShaderLanguage::WGSL;
    std::string source;
};

struct Shader : public Lattice::Component {
    using Desc = ShaderDesc;
    virtual ~Shader() = default;
};

}