#pragma once

#include "Surface.hpp"
#include "Pipeline.hpp"
#include <cstdint>

namespace GPU {

struct ClearColor { double r = 0.1, g = 0.2, b = 0.3, a = 1.0; };

class CommandList {
public:
    virtual ~CommandList() = default;
    virtual void draw(Surface& surface, Pipeline& pipeline, ClearColor clear, uint32_t vertices) = 0;
    virtual void submit() = 0;
};

}
