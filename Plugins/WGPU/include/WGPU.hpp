#pragma once

#include <span>
#include <vector>
#include <webgpu/webgpu.h>

#include "GPUAPI.hpp"

class NodeBuild;

namespace WGPU {

class WGPU final : public GPU::GPUAPI {
public:
    explicit WGPU(NodeBuild node, const Desc& = {});
    ~WGPU() override;

    WGPUDevice createDevice(WGPUAdapter& selectedAdapter);
    WGPUInstance native() const noexcept;

private:
    void createInstance();
    std::vector<WGPUAdapter> enumerateAdapters();
    WGPUAdapter selectAdapter(std::span<WGPUAdapter> adapters);

    WGPUInstance instance_ = nullptr;
};

}
