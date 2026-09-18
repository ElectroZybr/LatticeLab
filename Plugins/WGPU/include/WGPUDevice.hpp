#pragma once

#include <webgpu/webgpu.h>
#include <Lattice/Kernel/Node.hpp>
#include "Device.hpp"
#include "WGPU.hpp"

namespace WGPU {

class Device final : public GPU::Device {
public:
    explicit Device(Lattice::Node& node) {
        auto backend = node.requireParent<WGPU>();
        device_ = backend->createDevice();
        queue_ = wgpuDeviceGetQueue(device_);
    }
    
    ~Device() override {
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
