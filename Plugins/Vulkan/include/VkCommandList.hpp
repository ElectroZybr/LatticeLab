#pragma once

#include <vulkan/vulkan.h>

#include "CommandList.hpp"
#include <Lattice/Tools/Exception.hpp>

namespace Vk {

class CommandList final : public GPU::CommandList {
public:
    CommandList(VkDevice device, VkCommandPool pool, VkQueue queue)
        : device_(device), pool_(pool), queue_(queue)
    {
        VkCommandBufferAllocateInfo alloc{};
        alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc.commandPool = pool_;
        alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device_, &alloc, &buffer_) != VK_SUCCESS)
            throw Lattice::Exception("Vulkan::CommandList", "failed to allocate command buffer");

        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(buffer_, &begin) != VK_SUCCESS)
            throw Lattice::Exception("Vulkan::CommandList", "failed to begin command buffer");
    }

    ~CommandList() override {
        if (buffer_)
            vkFreeCommandBuffers(device_, pool_, 1, &buffer_);
    }

    void submit() override {
        if (!finished_) {
            if (vkEndCommandBuffer(buffer_) != VK_SUCCESS)
                throw Lattice::Exception("Vulkan::CommandList", "failed to end command buffer");

            finished_ = true;
        }

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &buffer_;

        if (vkQueueSubmit(queue_, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS)
            throw Lattice::Exception("Vulkan::CommandList", "failed to submit command buffer");

        vkQueueWaitIdle(queue_);
    }

    VkCommandBuffer native() const noexcept {
        return buffer_;
    }

    GPU::RenderPass& beginRenderPass(GPU::Surface&, GPU::Color = {}) override {
        throw Lattice::Exception("Vulkan::CommandList", "surface rendering is not implemented");
    }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkCommandPool pool_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;

    VkCommandBuffer buffer_ = VK_NULL_HANDLE;
    bool finished_ = false;
};

}
