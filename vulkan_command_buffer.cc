#include "vulkan_command_buffer.h"

VulkanCommandBuffer::~VulkanCommandBuffer() {
    if (cmd_buf_) vkFreeCommandBuffers(device_, cmd_pool_, 1, &cmd_buf_);
    if (fence_) vkDestroyFence(device_, fence_, nullptr);
    if (cmd_pool_) vkDestroyCommandPool(device_, cmd_pool_, nullptr);
}

int VulkanCommandBuffer::Create() {
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CALL(vkCreateFence(device_, &fenceInfo, nullptr, &fence_));

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = ctx_->QueueIndex();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CALL(vkCreateCommandPool(device_, &poolInfo, nullptr, &cmd_pool_));

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = cmd_pool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VK_CALL(vkAllocateCommandBuffers(device_, &allocInfo, &cmd_buf_));
    return 0;
}