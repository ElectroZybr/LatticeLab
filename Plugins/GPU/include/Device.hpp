#pragma once

#include <cstdint>
#include <string_view>
#include <string>

// #include "Buffer.hpp"
// #include "CommandList.hpp"
// #include "Queue.hpp"
// #include "Shader.hpp"
// #include "Texture.hpp"

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

class Device {
public:
    static constexpr std::string_view tag = "Device";

    virtual ~Device() = default;

    // virtual const DeviceInfo& info() const = 0;

    // virtual Buffer* createBuffer(const BufferDesc&) = 0;
    // virtual Texture* createTexture(const TextureDesc&) = 0;
    // virtual Shader* createShader(const ShaderDesc&) = 0;

    // virtual CommandList* createCommandList() = 0;
    // virtual Queue* queue() = 0;
};

}