#pragma once

#include <vulkan/vulkan.h>
#include <memory>

#include "Device.hpp"
#include "Vulkan.hpp"
#include "VkCommandList.hpp"

#include "Lattice/Kernel/Node.hpp"

namespace Vk {

class Device final : public GPU::Device {
public:
    explicit Device(Lattice::Node& node, const Desc& desc)
        : info_(desc)
    {
        auto backend = node.requireParent<Vulkan>();

        physicalDevice_ = backend->physicalDevice(desc.id);

        createDevice();
        createCommandPool();
    }

    ~Device() override {
        if (device_)
            vkDeviceWaitIdle(device_);

        if (commandPool_)
            vkDestroyCommandPool(device_, commandPool_, nullptr);

        if (device_)
            vkDestroyDevice(device_, nullptr);
    }

    std::unique_ptr<GPU::CommandList> createCommandList() override {
        return std::make_unique<CommandList>(device_, commandPool_, queue_);
    }

private:
    void createDevice();
    void createCommandPool();

    GPU::DeviceDesc info_;

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;

    uint32_t queueFamily_ = UINT32_MAX;
    VkQueue queue_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
};

}