#pragma once

#include <string_view>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include <Lattice/Kernel/Node.hpp>
#include "GPUAPI.hpp"
#include "WGPUDevice.hpp"

namespace WGPU {

class WGPU final : public GPU::GPUAPI {
    static constexpr std::string_view tag = "WGPU";
public:
    explicit WGPU(Lattice::Node& node) {
        createInstance();

        auto adapters = enumerateAdapters();

        WGPUAdapter adapter = selectAdapter(adapters);

        WGPUAdapterInfo info = {};
        wgpuAdapterGetInfo(adapter, &info);

        DeviceDesc deviceDesc{};
        deviceDesc.adapter = adapter;
        deviceDesc.instance = instance_;
        
        node.add<WDevice>(std::string(info.device.data, info.device.length), deviceDesc);
    }

private:
    void createInstance();
    std::vector<WGPUAdapter> enumerateAdapters();
    WGPUAdapter selectAdapter(std::span<WGPUAdapter> adapters);
    // WGPUDevice createDevice(WGPUAdapter adapter);

    WGPUInstance instance_ = nullptr;
};

}