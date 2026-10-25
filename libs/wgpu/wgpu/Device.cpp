#include "Device.hpp"
#include "CommandList.hpp"
#include "Resources.hpp"

#include <algorithm>
#include <atomic>
#include <format>
#include <string_view>
#include <webgpu/wgpu.h>

#include <Lattice/tools/Exception.hpp>
#include <lattice-logger/Logger.hpp>

namespace Wgpu {

namespace {
std::atomic<uint64_t> errors{0};
}

uint64_t errorCount() noexcept {
    return errors.load(std::memory_order_relaxed);
}

Instance::Instance() {
    WGPUInstanceDescriptor desc{};

    #ifndef NDEBUG
        WGPUInstanceExtras extras{};
        extras.chain.sType = static_cast<WGPUSType>(WGPUSType_InstanceExtras);
        extras.flags = WGPUInstanceFlag_Debug | WGPUInstanceFlag_Validation;
        desc.nextInChain = &extras.chain;
    #endif

    instance_ = wgpuCreateInstance(&desc);
    if (!instance_)
        throw Lattice::Exception<Instance>("failed to create instance");
}

Instance::~Instance() {
    if (instance_)
        wgpuInstanceRelease(instance_);
}

std::vector<WGPUAdapter> Instance::enumerateAdapters() {
    WGPUInstanceEnumerateAdapterOptions options{};
    options.backends = WGPUInstanceBackend_All;

    const size_t count = wgpuInstanceEnumerateAdapters(instance_, &options, nullptr);
    std::vector<WGPUAdapter> adapters(count);
    wgpuInstanceEnumerateAdapters(instance_, &options, adapters.data());
    adapters.resize(count);
    return adapters;
}

std::string Instance::adapterName() {
    auto adapters = enumerateAdapters();
    if (adapters.empty())
        throw Lattice::Exception<Instance>("no GPU adapters found");
    WGPUAdapterInfo info{};
    std::string name;
    if (wgpuAdapterGetInfo(adapters.front(), &info) == WGPUStatus_Success) {
        name.assign(info.device.data, info.device.length);
        wgpuAdapterInfoFreeMembers(info);
    }
    for (auto adapter : adapters)
        wgpuAdapterRelease(adapter);
    return name.empty() ? std::string{"device"} : name;
}

WGPUDevice Instance::createDevice(WGPUAdapter& selectedAdapter) {
    auto adapters = enumerateAdapters();
    struct AdapterCleanup {
        std::vector<WGPUAdapter>& adapters;
        ~AdapterCleanup() {
            for (auto adapter : adapters)
                wgpuAdapterRelease(adapter);
        }
    } cleanup{adapters};

    if (adapters.empty())
        throw Lattice::Exception<Instance>("no GPU adapters found");
    const auto adapter = adapters.front();

    // Timestamps (GPU time of passes and copies) when the adapter has them: measuring
    // tools use them, everything else works without.
    std::vector<WGPUFeatureName> features;
    for (const auto feature : {WGPUFeatureName_TimestampQuery,
                               static_cast<WGPUFeatureName>(WGPUNativeFeature_TimestampQueryInsideEncoders)})
        if (wgpuAdapterHasFeature(adapter, feature))
            features.push_back(feature);

    // The adapter's own limits instead of WebGPU's defaults (256 MB buffers, 128 MB storage
    // bindings): a table of millions of particles needs more.
    WGPULimits limits{};
    const bool haveLimits = wgpuAdapterGetLimits(adapter, &limits) == WGPUStatus_Success;
    limits.nextInChain = nullptr;

    WGPUDeviceDescriptor desc{};
    desc.requiredFeatureCount = features.size();
    desc.requiredFeatures = features.data();
    if (haveLimits)
        desc.requiredLimits = &limits;
    desc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    desc.deviceLostCallbackInfo.callback =
        [](WGPUDevice const*, WGPUDeviceLostReason reason, WGPUStringView message, void*, void*) {
            Logger::error("WGPU", "device lost ({}): {}", static_cast<int>(reason), std::string_view(message.data, message.length));
        };
    desc.uncapturedErrorCallbackInfo.callback =
        [](WGPUDevice const*, WGPUErrorType type, WGPUStringView message, void*, void*) {
            errors.fetch_add(1, std::memory_order_relaxed);
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
        throw Lattice::Exception<Instance>("failed to create device: {}", data.error);

    wgpuAdapterAddRef(adapter);
    selectedAdapter = adapter;
    return data.device;
}

uint64_t ResourceRegistry::add(std::string kind, std::string label, uint64_t bytes) {
    std::lock_guard lock(mutex_);
    const uint64_t id = next_++;
    live_.push_back({id, std::move(kind), std::move(label), bytes});
    return id;
}

void ResourceRegistry::remove(uint64_t id) noexcept {
    std::lock_guard lock(mutex_);
    std::erase_if(live_, [id](const ResourceInfo& info) { return info.id == id; });
}

std::vector<ResourceInfo> ResourceRegistry::snapshot() const {
    std::lock_guard lock(mutex_);
    return live_;
}

size_t ResourceRegistry::size() const {
    std::lock_guard lock(mutex_);
    return live_.size();
}

uint64_t ResourceRegistry::bytes() const {
    std::lock_guard lock(mutex_);
    uint64_t total = 0;
    for (const auto& info : live_)
        total += info.bytes;
    return total;
}

std::string ResourceRegistry::report() const {
    const auto rows = snapshot();
    size_t kindWidth = 4, labelWidth = 5;
    uint64_t total = 0;
    for (const auto& row : rows) {
        kindWidth = std::max(kindWidth, row.kind.size());
        labelWidth = std::max(labelWidth, row.label.size());
        total += row.bytes;
    }
    std::string text = std::format("{:>4}  {:<{}}  {:<{}}  {:>10}\n", "id", "kind", kindWidth, "label", labelWidth, "bytes");
    for (const auto& row : rows)
        text += std::format("{:>4}  {:<{}}  {:<{}}  {:>10}\n", row.id, row.kind, kindWidth, row.label, labelWidth,
            row.bytes ? std::to_string(row.bytes) : std::string{"-"});
    text += std::format("{} live, {} bytes", rows.size(), total);
    return text;
}

Device::Device(Instance& instance) {
    device_ = instance.createDevice(adapter_);
    instance_ = instance.native();
    wgpuInstanceAddRef(instance_);
    queue_ = wgpuDeviceGetQueue(device_);
}

Device::~Device() {
    if (const auto leaked = resources_.snapshot(); !leaked.empty()) {
        Logger::warning("WGPU", "device destroyed with {} live resources", leaked.size());
        for (const auto& info : leaked)
            Logger::warning("WGPU", "  {} '{}'", info.kind, info.label);
    }
    if (readback_)
        wgpuBufferRelease(readback_);
    if (queue_)
        wgpuQueueRelease(queue_);
    if (device_)
        wgpuDeviceRelease(device_);
    if (adapter_)
        wgpuAdapterRelease(adapter_);
    if (instance_)
        wgpuInstanceRelease(instance_);
}

void Device::writeBuffer(Buffer& buffer, uint64_t offset, std::span<const std::byte> data) {
    wgpuQueueWriteBuffer(queue_, buffer.native(), offset, data.data(), data.size());
}

bool Device::has(Feature feature) const noexcept {
    switch (feature) {
        case Feature::Timestamps:
            return wgpuDeviceHasFeature(device_, WGPUFeatureName_TimestampQuery);
        case Feature::EncoderTimestamps:
            return wgpuDeviceHasFeature(device_, static_cast<WGPUFeatureName>(WGPUNativeFeature_TimestampQueryInsideEncoders));
    }
    return false;
}

DeviceLimits Device::limits() const noexcept {
    WGPULimits limits{};
    if (wgpuDeviceGetLimits(device_, &limits) != WGPUStatus_Success)
        return {};
    return {.maxBufferSize = limits.maxBufferSize, .maxStorageBinding = limits.maxStorageBufferBindingSize,
            .maxWorkgroupsPerDimension = limits.maxComputeWorkgroupsPerDimension};
}

double Device::timestampPeriod() const noexcept {
    return double(wgpuQueueGetTimestampPeriod(queue_));
}

void Device::wait() {
    wgpuDevicePoll(device_, true, nullptr);
}

void Device::flush() {
    CommandList{*this}.submit();
    wait();
}

void Device::readBuffer(Buffer& buffer, uint64_t offset, std::span<std::byte> out) {
    const uint64_t size = out.size();
    if (offset % 4 || size % 4)
        throw Lattice::Exception<Device>("readBuffer: offset {} and size {} must be multiples of 4", offset, size);
    if (offset + size > buffer.size())
        throw Lattice::Exception<Device>("readBuffer: {} bytes at {} exceed the buffer of {}", size, offset, buffer.size());
    if (!size)
        return;

    std::lock_guard lock(readbackMutex_);
    if (!readback_ || readbackSize_ < size) {
        if (readback_)
            wgpuBufferRelease(readback_);
        WGPUBufferDescriptor desc{};
        desc.label = {"readback", 8};
        desc.size = std::max(size, readbackSize_ * 2);
        desc.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
        readback_ = wgpuDeviceCreateBuffer(device_, &desc);
        if (!readback_)
            throw Lattice::Exception<Device>("readBuffer: cannot create a staging buffer of {} bytes", desc.size);
        readbackSize_ = desc.size;
    }
    CommandList commands{*this};
    wgpuCommandEncoderCopyBufferToBuffer(commands.encoder(), buffer.native(), offset, readback_, 0, size);
    commands.submit();

    struct Mapping {
        std::atomic<bool> done = false;
        WGPUMapAsyncStatus status = WGPUMapAsyncStatus_Error;
    } mapping;
    WGPUBufferMapCallbackInfo callback{};
    callback.mode = WGPUCallbackMode_AllowProcessEvents;
    callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void* data, void*) {
        auto& self = *static_cast<Mapping*>(data);
        self.status = status;
        self.done = true;
    };
    callback.userdata1 = &mapping;
    wgpuBufferMapAsync(readback_, WGPUMapMode_Read, 0, size, callback);
    while (!mapping.done) {
        wgpuDevicePoll(device_, true, nullptr);
        wgpuInstanceProcessEvents(instance_);
    }
    if (mapping.status != WGPUMapAsyncStatus_Success)
        throw Lattice::Exception<Device>("readBuffer: mapping failed ({})", static_cast<int>(mapping.status));
    const auto* data = static_cast<const std::byte*>(wgpuBufferGetConstMappedRange(readback_, 0, size));
    std::copy(data, data + size, out.begin());
    wgpuBufferUnmap(readback_);
}

std::vector<std::byte> Device::readBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    std::vector<std::byte> result(size);
    readBuffer(buffer, offset, result);
    return result;
}

}
