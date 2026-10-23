#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <string>
#include <vector>
#include <webgpu/webgpu.h>

namespace WGPU {

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

class Device final {
public:
    explicit Device(Instance& instance);
    ~Device();
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    void writeBuffer(Buffer& buffer, uint64_t offset, std::span<const std::byte> data);

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
};

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
