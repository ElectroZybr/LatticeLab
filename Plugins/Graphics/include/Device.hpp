#pragma once
#include <GPU/include/Device.hpp>
#include "CommandList.hpp"

namespace Graphics {
class Device : public GPU::Device {
public:
    virtual std::unique_ptr<CommandList> createRenderCommandList() = 0;
};
}
