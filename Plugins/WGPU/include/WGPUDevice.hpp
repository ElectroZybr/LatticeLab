#pragma once

#include <webgpu/webgpu.h>
#include <Lattice/Kernel/Node.hpp>
#include "Device.hpp"
#include "WGPU.hpp"

namespace WGPU {

class WDevice final : public GPU::Device {
public:
    explicit WDevice(Lattice::Node& node) {
        auto backend = node.requireParent<WGPU>();
        device_ = backend->createDevice();
        queue_ = wgpuDeviceGetQueue(device_);
    }
    
    ~WDevice() override {
        if (queue_)
            wgpuQueueRelease(queue_);
        if (device_)
            wgpuDeviceRelease(device_);
    }


private:
    WGPUDevice device_ = nullptr;
    WGPUQueue queue_ = nullptr;
};

}
