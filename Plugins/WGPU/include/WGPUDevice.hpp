#pragma once

#include <string_view>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include "Lattice/Kernel/Node.hpp"
#include "Device.hpp"

namespace WGPU {

struct DeviceDesc {
    WGPUInstance instance = nullptr;
    WGPUAdapter adapter = nullptr;
};

class WDevice final : public GPU::Device {
    static constexpr std::string_view tag = "WGPUDevice";
public:
    using Desc = DeviceDesc;
    explicit WDevice(Lattice::Node& nodec, DeviceDesc deviceDesc) {
        device_ = createDevice(deviceDesc);
        queue_ = wgpuDeviceGetQueue(device_);
    }

    ~WDevice() = default;

    WGPUDevice createDevice(DeviceDesc& deviceDesc) {
        if (!deviceDesc.adapter)
            throw Lattice::Exception(tag, "invalid adapter");

        WGPUDeviceDescriptor desc = {};

        desc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
        desc.deviceLostCallbackInfo.callback =
            [](WGPUDevice const*, WGPUDeviceLostReason reason, WGPUStringView message, void*, void*) {
                Logger::error("WGPU", "device lost ({}): {}", static_cast<int>(reason), std::string_view(message.data, message.length));
            };

        desc.uncapturedErrorCallbackInfo.callback =
            [](WGPUDevice const*, WGPUErrorType type, WGPUStringView message, void*, void*) {
                Logger::error("WGPU", "error ({}): {}", static_cast<int>(type), std::string_view(message.data, message.length));
            };

        struct UserData {
            WGPUDevice device = nullptr;
            std::string error;
            bool done = false;
        } data;

        WGPURequestDeviceCallbackInfo callback = {};
        callback.mode = WGPUCallbackMode_AllowSpontaneous;
        callback.callback =
            [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata1, void*) {
                auto* data = static_cast<UserData*>(userdata1);

                if (status == WGPURequestDeviceStatus_Success)
                    data->device = device;
                else
                    data->error = std::string(message.data, message.length);

                data->done = true;
            };
        callback.userdata1 = &data;

        wgpuAdapterRequestDevice(deviceDesc.adapter, &desc, callback);

        while (!data.done)
            wgpuInstanceProcessEvents(deviceDesc.instance);

        if (!data.device)
            throw Lattice::Exception(tag, "failed to create device: {}", data.error);

        return data.device;
    }

private:
    WGPUDevice device_ = nullptr;
    WGPUQueue queue_ = nullptr;
};

}