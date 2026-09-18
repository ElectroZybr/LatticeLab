#pragma once

#include "CommandList.hpp"
#include <Lattice/Kernel/Component.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace GPU {

enum class DeviceType {
    Discrete,
    Integrated,
    Virtual,
    CPU
};

struct DeviceInfo {
    std::string name;
    DeviceType type;
    uint64_t memory = 0;
};

class Device : public Lattice::Component {
public:
    virtual ~Device() = default;
    virtual std::unique_ptr<GPU::CommandList> createCommandList() = 0;
};

}