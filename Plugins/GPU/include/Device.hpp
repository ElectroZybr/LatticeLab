#pragma once

#include "CommandList.hpp"
#include <Lattice/Kernel/Component.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace GPU {

enum class DeviceType {
    Discrete,
    Integrated,
    Virtual,
    CPU,
    Unknown
};

using DeviceId = uint32_t;

struct DeviceDesc {
    uint32_t id = 0;
    std::string name;
    DeviceType type = DeviceType::Unknown;
    uint64_t memory = 0;
};

class Device : public Lattice::Component {
public:
    using Desc = DeviceDesc;
    virtual ~Device() = default;
    virtual std::unique_ptr<GPU::CommandList> createCommandList() = 0;
    virtual std::unique_ptr<BindingSet> createBindingSet(Pipeline& pipeline, uint32_t group, std::span<const Binding> bindings) = 0;
    virtual void writeBuffer(Buffer& buffer, uint64_t offset, std::span<const std::byte> data) = 0;
};

}