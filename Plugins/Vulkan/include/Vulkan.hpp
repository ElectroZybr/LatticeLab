#pragma once

#include <span>
#include <vector>

#include <vulkan/vulkan.h>

#include "VkDevice.hpp"
#include "GPUAPI.hpp"

#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/Node.hpp>

namespace Vk {

class Vulkan final : public GPU::GPUAPI {
    static constexpr std::string_view tag = "Vulkan";

public:
    explicit Vulkan(Lattice::Node&, const Desc& = {}) {
        createInstance();
    }

    void configure(Lattice::Node& node) {
        physicalDevices_ = enumeratePhysicalDevices();

        VkPhysicalDevice physical = selectPhysicalDevice(physicalDevices_);

        const GPU::DeviceDesc desc = describeDevice(physical);

        node.add<Device>(desc.name, desc);
    }

    ~Vulkan() override {
        if (instance_)
            vkDestroyInstance(instance_, nullptr);
    }

    VkDevice createDevice(VkPhysicalDevice physical);

    VkPhysicalDevice physicalDevice(uint32_t id) const {
        if (id >= physicalDevices_.size())
            throw Lattice::Exception(tag, "invalid physical device id '{}'", id);

        return physicalDevices_[id];
    }

    VkInstance native() const noexcept {
        return instance_;
    }

private:
    void createInstance();

    std::vector<VkPhysicalDevice> enumeratePhysicalDevices();
    VkPhysicalDevice selectPhysicalDevice(std::span<VkPhysicalDevice> devices);

    GPU::DeviceDesc describeDevice(VkPhysicalDevice physical) const;

    static GPU::DeviceType deviceType(VkPhysicalDeviceType type) {
        switch (type) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return GPU::DeviceType::Discrete;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return GPU::DeviceType::Integrated;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return GPU::DeviceType::Virtual;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            return GPU::DeviceType::CPU;
            default:                                     return GPU::DeviceType::Unknown;
        }
    }

    VkInstance instance_ = VK_NULL_HANDLE;
    std::vector<VkPhysicalDevice> physicalDevices_;
};

}
