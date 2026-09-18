#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include <Lattice/Kernel/Node.hpp>
#include "GPUAPI.hpp"

namespace WGPU {

class Device;

class WGPU final : public GPU::GPUAPI {
    static constexpr std::string_view tag = "WGPU";
public:
    explicit WGPU(Lattice::Node&) { createInstance(); }

    void configure(Lattice::Node& node) {
        node.add<Device>(deviceName());
    }

    ~WGPU() override {
        if (instance_)
            wgpuInstanceRelease(instance_);
    }

    WGPUDevice createDevice();
private:
    std::string deviceName();
    void createInstance();
    std::vector<WGPUAdapter> enumerateAdapters();
    WGPUAdapter selectAdapter(std::span<WGPUAdapter> adapters);

    WGPUInstance instance_ = nullptr;
};

}