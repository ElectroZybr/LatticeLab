#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <string>
#include <vector>
#include <webgpu/webgpu.h>

namespace Wgpu {

class Buffer;

/// Owns the native instance and selects adapters.
class Instance final {
public:
    Instance();
    ~Instance();
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    /// Request a device from the first available adapter; returns it with `adapter` set.
    WGPUDevice createDevice(WGPUAdapter& adapter);
    /// Name of the adapter createDevice selects, e.g. "NVIDIA GeForce RTX 3060".
    std::string adapterName();
    WGPUInstance native() const noexcept { return instance_; }

private:
    std::vector<WGPUAdapter> enumerateAdapters();

    WGPUInstance instance_ = nullptr;
};

struct ResourceInfo {
    uint64_t id = 0;
    std::string kind;
    std::string label;
    uint64_t bytes = 0;
};

/// Live GPU resources of one device. Thread-safe: resources are created on the render
/// thread while tooling reads snapshots from elsewhere.
class ResourceRegistry final {
public:
    uint64_t add(std::string kind, std::string label, uint64_t bytes);
    void remove(uint64_t id) noexcept;
    [[nodiscard]] std::vector<ResourceInfo> snapshot() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] uint64_t bytes() const;
    /// Snapshot rendered as a text table.
    [[nodiscard]] std::string report() const;

private:
    mutable std::mutex mutex_;
    std::vector<ResourceInfo> live_;
    uint64_t next_ = 1;
};

/// Optional device capabilities, requested at creation when the adapter has them.
enum class Feature {
    Timestamps,         // GPU time at the beginning and end of passes (Timer)
    EncoderTimestamps   // GPU time between commands of an encoder (copies)
};

/// What the device allows (requested at creation as the adapter's maxima).
struct DeviceLimits {
    uint64_t maxBufferSize = 0;
    uint64_t maxStorageBinding = 0;
    uint32_t maxWorkgroupsPerDimension = 0;
};

class Device final {
public:
    explicit Device(Instance& instance);
    ~Device();
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    void writeBuffer(Buffer& buffer, uint64_t offset, std::span<const std::byte> data);

    /// Copy `size` bytes of a GPU buffer (CopySource) back to the host; waits for the
    /// GPU. The mappable staging buffer is the device's own and is kept (creating one
    /// costs ~1 ms); it grows to the largest read.
    std::vector<std::byte> readBuffer(Buffer& buffer, uint64_t offset, uint64_t size);
    /// The same into the caller's memory (`out.size()` bytes): no allocation per read.
    void readBuffer(Buffer& buffer, uint64_t offset, std::span<std::byte> out);

    /// Wait until the GPU has finished all submitted work.
    void wait();

    /// Submit what writeBuffer has staged and wait for it: writeBuffer only copies into a
    /// staging area, the GPU copies to the buffer at the next submit. Measuring code calls
    /// this to keep a transfer inside its own phase.
    void flush();

    bool has(Feature feature) const noexcept;
    DeviceLimits limits() const noexcept;
    /// Nanoseconds per timestamp tick.
    double timestampPeriod() const noexcept;

    WGPUDevice native() const noexcept { return device_; }
    WGPUAdapter adapter() const noexcept { return adapter_; }
    WGPUInstance instance() const noexcept { return instance_; }
    WGPUQueue queue() const noexcept { return queue_; }

    ResourceRegistry& resources() noexcept { return resources_; }
    const ResourceRegistry& resources() const noexcept { return resources_; }

private:
    WGPUAdapter adapter_ = nullptr;
    WGPUInstance instance_ = nullptr;
    WGPUDevice device_ = nullptr;
    WGPUQueue queue_ = nullptr;
    ResourceRegistry resources_;
    // readBuffer's staging buffer: device-internal, not a registered resource.
    std::mutex readbackMutex_;
    WGPUBuffer readback_ = nullptr;
    uint64_t readbackSize_ = 0;
};

/// Uncaptured WGPU errors (validation, out of memory) of all devices since start.
/// Errors are also logged; tests compare the count before and after.
uint64_t errorCount() noexcept;

/// RAII registration of a resource in its device's registry.
class Tracked {
public:
    Tracked(Device& device, std::string kind, std::string label, uint64_t bytes)
        : device_(device), id_(device.resources().add(std::move(kind), std::move(label), bytes)) {}
    ~Tracked() { device_.resources().remove(id_); }
    Tracked(const Tracked&) = delete;
    Tracked& operator=(const Tracked&) = delete;

    Device& device() const noexcept { return device_; }

private:
    Device& device_;
    uint64_t id_;
};

}
