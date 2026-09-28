#pragma once

#include <vulkan/vulkan.h>
#include <memory>

#include "Device.hpp"
#include "VkCommandList.hpp"
#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Kernel/NodeViews.hpp>

namespace Vk {
class Vulkan;

class Device final : public GPU::Device {
public:
    explicit Device(NodeBuild, const Desc& desc = {}) : info_(desc) {}
    void configure(NodeBuild node);

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

    std::unique_ptr<GPU::Surface> createSurface(const GPU::SurfaceDesc&) override {
        throw Lattice::Exception<Device>("createSurface is not implemented");
    }

    std::unique_ptr<GPU::Shader> createShader(const GPU::ShaderDesc&) override {
        throw Lattice::Exception<Device>("createShader is not implemented");
    }

    std::unique_ptr<GPU::Pipeline> createPipeline(const GPU::PipelineDesc&) override {
        throw Lattice::Exception<Device>("createPipeline is not implemented");
    }

    std::unique_ptr<GPU::Buffer> createBuffer(const GPU::BufferDesc&) override {
        throw Lattice::Exception<Device>("createBuffer is not implemented");
    }

    std::unique_ptr<GPU::BindingSet> createBindingSet(GPU::Pipeline&, uint32_t, std::span<const GPU::Binding>) override {
        throw Lattice::Exception<Device>("createBindingSet is not implemented");
    }

    void writeBuffer(GPU::Buffer&, uint64_t, std::span<const std::byte>) override {
        throw Lattice::Exception<Device>("writeBuffer is not implemented");
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
