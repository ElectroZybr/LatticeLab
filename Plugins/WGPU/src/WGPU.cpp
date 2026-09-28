#include <atomic>
#include <string>
#include <string_view>
#include <webgpu/wgpu.h>

#include "WGPU.hpp"
#include "WGPUDevice.hpp"

#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Tools/Logger.hpp>

namespace WGPU {

WGPU::WGPU(NodeBuild node, const Desc&) {
    createInstance();
    auto device = node.add<GPU::Device>();
}

WGPU::~WGPU() {
    if (instance_)
        wgpuInstanceRelease(instance_);
}

WGPUInstance WGPU::native() const noexcept {
    return instance_;
}

void WGPU::createInstance() {
    WGPUInstanceDescriptor desc{};

    #ifndef NDEBUG
        WGPUInstanceExtras extras{};
        extras.chain.sType = static_cast<WGPUSType>(WGPUSType_InstanceExtras);
        extras.flags = WGPUInstanceFlag_Debug | WGPUInstanceFlag_Validation;
        desc.nextInChain = &extras.chain;
    #endif

    instance_ = wgpuCreateInstance(&desc);
    if (!instance_)
        throw Lattice::Exception<WGPU>("failed to create instance");
}

std::vector<WGPUAdapter> WGPU::enumerateAdapters() {
    WGPUInstanceEnumerateAdapterOptions options{};
    options.backends = WGPUInstanceBackend_All;

    const size_t count = wgpuInstanceEnumerateAdapters(instance_, &options, nullptr);

    std::vector<WGPUAdapter> adapters(count);
    wgpuInstanceEnumerateAdapters(instance_, &options, adapters.data());

    adapters.resize(count);
    return adapters;
}

WGPUAdapter WGPU::selectAdapter(std::span<WGPUAdapter> adapters) {
    if (adapters.empty())
        throw Lattice::Exception<WGPU>("no GPU adapters found");

    return adapters.front();
}

WGPUDevice WGPU::createDevice(WGPUAdapter& selectedAdapter) {
    auto adapters = enumerateAdapters();
    struct AdapterCleanup {
        std::vector<WGPUAdapter>& adapters;
        ~AdapterCleanup() {
            for (auto adapter : adapters)
                wgpuAdapterRelease(adapter);
        }
    } cleanup{adapters};
    const auto adapter = selectAdapter(adapters);
    WGPUDeviceDescriptor desc{};

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
        std::atomic_bool done = false;
    } data;

    WGPURequestDeviceCallbackInfo callback{};
    callback.mode = WGPUCallbackMode_AllowSpontaneous;
    callback.callback =
        [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata1, void*) {
            auto* data = static_cast<UserData*>(userdata1);

            if (status == WGPURequestDeviceStatus_Success)
                data->device = device;
            else
                data->error = std::string(message.data, message.length);

            data->done.store(true, std::memory_order_release);
        };
    callback.userdata1 = &data;

    wgpuAdapterRequestDevice(adapter, &desc, callback);

    while (!data.done.load(std::memory_order_acquire))
        wgpuInstanceProcessEvents(instance_);

    if (!data.device)
        throw Lattice::Exception<WGPU>("failed to create device: {}", data.error);

    wgpuAdapterAddRef(adapter);
    selectedAdapter = adapter;
    return data.device;
}

}
