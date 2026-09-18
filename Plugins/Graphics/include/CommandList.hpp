#pragma once
#include <GPU/include/CommandList.hpp>
#include "Surface.hpp"
#include "RenderPipeline.hpp"

namespace Graphics {
class CommandList : public GPU::CommandList {
public:
    virtual void draw(Surface& surface, RenderPipeline& pipeline, ClearColor clear, uint32_t vertices) = 0;
};
}
